#include "client.h"
#include "../common/common.h"
#include "ui.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>

static StatsMessage send_command(MessageType type, int x, int y) {
    StatsMessage stats;
    memset(&stats, 0, sizeof(stats));   //VEĽMI DÔLEŽITÉ

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

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

void* receiver_thread_func(void* arg) {
    ClientContext* ctx = (ClientContext*)arg;
    while (ctx->keep_running) {
        pthread_mutex_lock(&ctx->mutex);
        UIState current = ctx->current_state;
        pthread_mutex_unlock(&ctx->mutex);

        // Vlákno pracuje IBA ak je simulácia aktívna (nie v Menu ani v Setup)
        if (current == UI_INTERACTIVE || current == UI_SUMMARY) {
            StatsMessage new_data = send_command(MSG_SIM_GET_STATS, 0, 0); 
            
            pthread_mutex_lock(&ctx->mutex);
            // Ak sa stav medzičasom nezmenil na Menu, ulož dáta
            if (ctx->current_state == current) {
                ctx->stats = new_data;
            }
            pthread_mutex_unlock(&ctx->mutex);
        } else {
            // V menu alebo setupe vlákno len čaká a nič neposiela
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

    while (ctx.current_state != UI_EXIT) {
        // Aktualizujeme stav v kontexte pre vlákno
        pthread_mutex_lock(&ctx.mutex);
        UIState local_state = ctx.current_state;
        pthread_mutex_unlock(&ctx.mutex);

        switch (local_state) {

        case UI_MENU_MODE:
            // V menu vynulujeme staré štatistiky
            pthread_mutex_lock(&ctx.mutex);
            memset(&ctx.stats, 0, sizeof(ctx.stats));
            pthread_mutex_unlock(&ctx.mutex);

            x = 5, y = 5, K = 100, runs = 1 , height = 11, width = 11;
            UIState next = draw_mode_menu(&mode);
            
            if (next == UI_SETUP_SIM) {
                // P3 & P5: Vytvorenie procesu servera
                pid_t pid = fork();
                if (pid == 0) {
                    setsid();
                    execl("./server_app", "./server_app", NULL);
                    exit(1);
                }
                usleep(150000); // Pauza pre server
            }
            
            pthread_mutex_lock(&ctx.mutex);
            ctx.current_state = next;
            pthread_mutex_unlock(&ctx.mutex);
            break;

        case UI_SETUP_SIM:
            draw_setup(&x, &y, &K, &runs, &width, &height, probs, mode);
            
            // Odoslanie konfigurácie (rovnako ako vo tvojom kóde)
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            struct sockaddr_un addr = {0};
            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, SOCKET_PATH);
            
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                Message configMsg = {
                    .type = MSG_SIM_CONFIG, .x = x, .y = y,
                    .width = width, .height = height,
                    .max_steps = K, .replications = runs
                };
                memcpy(configMsg.probs, probs, sizeof(probs));
                write(fd, &configMsg, sizeof(configMsg));
                
                // Prvotné načítanie štatistík
                StatsMessage temp_stats;
                memset(&temp_stats, 0 , sizeof(temp_stats));
                int got = 0;
                while (got < sizeof(temp_stats)) {
                    int r = read(fd, ((char*)&temp_stats) + got, sizeof(temp_stats) - got);
                    if (r <= 0) break;
                    got += r;
                }
                
                pthread_mutex_lock(&ctx.mutex);
                ctx.stats = temp_stats;
                ctx.current_state = (mode == 1) ? UI_INTERACTIVE : UI_SUMMARY;
                pthread_mutex_unlock(&ctx.mutex);
                close(fd);
            }
            break;

        case UI_INTERACTIVE:
        case UI_SUMMARY:
            clear();
            
            // Zamkneme dáta pre bezpečné vykreslenie (P11)
            pthread_mutex_lock(&ctx.mutex);
            StatsMessage current_stats = ctx.stats;
            pthread_mutex_unlock(&ctx.mutex);

            if (local_state == UI_INTERACTIVE) {
                mvprintw(1, 2, "INTERAKTIVNY MOD | Start: (%d,%d)", x, y);
                mvprintw(2, 2, "r - krok, c - reset, q - menu");
                draw_world(current_stats.height, current_stats.width, current_stats.posX, current_stats.posY, current_stats.obstacle, current_stats.visited);
                draw_stats(&current_stats, 3 + current_stats.height + 2, local_state);
            } else {
                mvprintw(1, 2, "SUMARNY MOD | K=%d, replikacie=%d", K, runs);
                mvprintw(2, 2, "r - spustit, c - reset, q - menu");
                draw_stats(&current_stats, 5, local_state);
            }

            refresh();
            
            // timeout(50) spôsobí, že getch() nečaká večne a UI sa plynule obnovuje
            timeout(50); 
            int ch = getch();

            if (ch == 'r') {
                send_command(local_state == UI_INTERACTIVE ? MSG_SIM_STEP : MSG_SIM_RUN, x, y);
            } else if (ch == 'c') {
                send_command(MSG_SIM_RESET, x, y);
            } else if (ch == 'q') {
                pthread_mutex_lock(&ctx.mutex);
                ctx.current_state = UI_MENU_MODE;
                memset(&ctx.stats, 0, sizeof(ctx.stats));
                pthread_mutex_unlock(&ctx.mutex);
            }
            break;

        default:
            ctx.current_state = UI_EXIT;
            break;
        }
    }

    // Korektné ukončenie (P5 - server beží ďalej, ale klient končí čisto)
    ctx.keep_running = 0;
    pthread_join(receiver_tid, NULL);
    pthread_mutex_destroy(&ctx.mutex);
    endwin();
}
