#include "client.h"
#include "../common/common.h"
#include "../common/ipc.h"
#include "ui.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>

static StatsMessage send_command(const char* socket_path,MessageType type, int x, int y) {
    StatsMessage stats;
    memset(&stats, 0, sizeof(stats));   //VEĽMI DÔLEŽITÉ

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path , sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return stats;
    }

    Message msg = { .type = type, .x = x, .y = y };
    write(fd, &msg, sizeof(msg));

    //SPRÁVNE ČÍTANIE CELÉHO STRUCTU
    int got = 0;
    while (got < sizeof(stats)) {
        int r = read(fd, ((char*)&stats) + got,
                     sizeof(stats) - got);
        if (r <= 0) {
            perror("read");
            break;
        }
        got += r;
    }

    close(fd);
    return stats;
}

void* receiver_thread_func(void* arg)
{
    ClientContext* ctx = (ClientContext*)arg;

    while (ctx->keep_running) {

        // 1️⃣ Zistíme aktuálny stav UI
        pthread_mutex_lock(&ctx->mutex);
        UIState current = ctx->current_state;
        pthread_mutex_unlock(&ctx->mutex);

        // 2️⃣ Pollujeme len počas simulácie
        if (current == UI_INTERACTIVE || current == UI_SUMMARY) {

            StatsMessage new_data =
                send_command(ctx->active_socket_path,
                             MSG_SIM_GET_STATS,
                             0, 0);

            // 3️⃣ DETEKCIA NEPLATNEJ ODPOVEDE
            int valid =
                new_data.width  != 0 &&
                new_data.height != 0;

            if (valid) {

                pthread_mutex_lock(&ctx->mutex);

                // Stav sa nezmenil?
                if (ctx->current_state == current) {

                    ctx->stats = new_data;

                    // 4️⃣ Koniec simulácie → STOP
                    if (new_data.finished) {
                        ctx->keep_running = 0;
                        pthread_mutex_unlock(&ctx->mutex);
                        break;
                    }
                }

                pthread_mutex_unlock(&ctx->mutex);
            }
            // ❗ neplatné dáta → IGNORUJEME

        } else {
            usleep(500000);
        }

        usleep(100000);
    }

    return NULL;
}

void client_run(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // Inicializácia zdieľaného kontextu (bez globálnych premenných)
    ClientContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    ctx.keep_running = 1;
    ctx.current_state = UI_MENU_MODE;

    // Lokálne konfiguračné premenné
    int mode = 0;
    int x = 5, y = 5, K = 100, runs = 1;
    int probs[4] = {25, 25, 25, 25};
    int height = 11, width = 11;

    // Spustenie prijímacieho vlákna (P11)
    pthread_t receiver_tid;
    pthread_create(&receiver_tid, NULL, receiver_thread_func, &ctx);

   while (1) {

    pthread_mutex_lock(&ctx.mutex);
    UIState local_state = ctx.current_state;
    pthread_mutex_unlock(&ctx.mutex);

    switch (local_state) {

   // =========================
// MENU
// =========================
// V client_run prepis cast case UI_MENU_MODE:
case UI_MENU_MODE: {
    
    timeout(-1);
    flushinp();
        
    pthread_mutex_lock(&ctx.mutex);
    memset(&ctx.stats, 0, sizeof(ctx.stats));
    pthread_mutex_unlock(&ctx.mutex);

    char room_code[16] = {0};
    int conn_choice = draw_connection_menu(room_code);

    if (conn_choice == 0) break; // Pouzivatel stlacil nieco ine

    // Vytvorime unikatnu cestu k socketu podla kodu
    char current_socket_path[256] = {0};

    if (conn_choice == 1) { // VYTVORIT NOVU
        sprintf(current_socket_path, "/tmp/drunk_%s.sock", room_code);
        strncpy(ctx.active_socket_path, current_socket_path, 255);
        
        // Tu este potrebujeme vediet, ci to bude Interaktivny alebo Sumarny mod
        ctx.current_state = draw_mode_menu(&mode); 

        if (ctx.current_state == UI_SETUP_SIM) {
            pid_t pid = fork();
            if (pid == 0) {
                setsid();
                // Serveru odovzdame cestu k socketu ako argument
                execl("./server_app", "./server_app", current_socket_path, NULL);
                exit(1);
            }
            usleep(250000); // Cas pre server na bind()
        }
    } 
    else if (conn_choice == 2) { // PRIPOJIT SA - Výber z registra (P10)
        if (draw_server_list_menu(current_socket_path)) {
            strncpy(ctx.active_socket_path, current_socket_path, 255);
            // Ak existuje, ideme rovno do simulacie (vsetko si stiahne receiver_thread)
            ctx.current_state = UI_INTERACTIVE; 
        } else {
            ctx.current_state = UI_MENU_MODE;
        }
    }
    break;
}
    // =========================
    // SETUP
    // =========================
    case UI_SETUP_SIM: {
        UIState next = draw_setup(
            &x, &y, &K, &runs,
            &width, &height,
            probs, mode
        );

        // 🔹 stále editujem → nič nerob
        if (next == UI_SETUP_SIM) {
            break;
        }

        // 🔹 návrat do menu
        if (next == UI_MENU_MODE) {
            pthread_mutex_lock(&ctx.mutex);
            ctx.current_state = UI_MENU_MODE;
            pthread_mutex_unlock(&ctx.mutex);
            break;
        }

        // 🔹 potvrdený setup → pošli konfiguráciu
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path,ctx.active_socket_path );

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {

            Message configMsg = {
                .type = MSG_SIM_CONFIG,
                .x = x, .y = y,
                .width = width, .height = height,
                .max_steps = K, .replications = runs
            };
            memcpy(configMsg.probs, probs, sizeof(probs));
            write(fd, &configMsg, sizeof(configMsg));

            StatsMessage temp_stats = {0};
            int got = 0;
            while (got < sizeof(temp_stats)) {
                int r = read(fd,
                    ((char*)&temp_stats) + got,
                    sizeof(temp_stats) - got);
                if (r <= 0) break;
                got += r;
            }

            pthread_mutex_lock(&ctx.mutex);
            ctx.stats = temp_stats;
            ctx.current_state = next;
            pthread_mutex_unlock(&ctx.mutex);

            close(fd);
        }
        break;
    }

    // =========================
    // INTERACTIVE + SUMMARY
    // =========================
    case UI_INTERACTIVE:
    case UI_SUMMARY: {
        clear();

        pthread_mutex_lock(&ctx.mutex);
        StatsMessage current_stats = ctx.stats;
        pthread_mutex_unlock(&ctx.mutex);

        if (local_state == UI_INTERACTIVE) {
            mvprintw(1, 2, "INTERAKTIVNY MOD | Start: (%d,%d)", x, y);
            mvprintw(2, 2, "r - krok, c - reset, q - menu");
            draw_world(
                current_stats.height,
                current_stats.width,
                current_stats.posX,
                current_stats.posY,
                current_stats.obstacle,
                current_stats.visited
            );
            draw_stats(&current_stats,
                3 + current_stats.height + 2,
                local_state);
        } else {
            mvprintw(1, 2, "SUMARNY MOD | K=%d, replikacie=%d", K, runs);
            mvprintw(2, 2, "r - spustit, c - reset, q - menu \n");
            draw_stats(&current_stats, 5, local_state);
        }

        refresh();
        timeout(50);
        int ch = getch();

        if (ch == 'r') {
            send_command(ctx.active_socket_path ,
                local_state == UI_INTERACTIVE
                    ? MSG_SIM_STEP
                    : MSG_SIM_RUN,
                x, y
            );
        } else if (ch == 'c') {
            send_command(ctx.active_socket_path ,MSG_SIM_RESET, x, y);
        } else if (ch == 'q') {
            pthread_mutex_lock(&ctx.mutex);
            ctx.current_state = UI_MENU_MODE;
            memset(&ctx.stats, 0, sizeof(ctx.stats));
            pthread_mutex_unlock(&ctx.mutex);
        }
        break;
    }

    // =========================
    // EXIT
    // =========================
    case UI_EXIT:
        return;
    }
}

    // Korektné ukončenie (P5 - server beží ďalej, ale klient končí čisto)
    ctx.keep_running = 0;
    pthread_join(receiver_tid, NULL);
    pthread_mutex_destroy(&ctx.mutex);
    endwin();
}
