/**
 * ============================================================================
 * FILE: client/client.c - KLIENT APLIKÁCIE RANDOM WALK
 * ============================================================================
 * Spravuje UI, komunikáciu so serverom a vlákna pre paralelné čítanie.
 * Architekúra: main thread (UI) + receiver_thread (dáta) + input_thread (vstupy)
 */

#include "client.h"
#include "../common/common.h"
#include "../common/ipc.h"
#include "ui.h"
#include "menu_handler.h"
#include "simulation_handler.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>

// Pošle príkaz na server a čaká odpoveď
StatsMessage send_command(const char* socket_path, MessageType type, int x, int y) {
    StatsMessage stats;
    memset(&stats, 0, sizeof(stats));

    // Vytvoríme socket
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    // Pripravíme adresu servera
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path , sizeof(addr.sun_path) - 1);

    // Pokus o pripojenie
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return stats; // Chyba - vrátime prázdne štatistiky
    }

    // Vytvoríme a pošleme správu
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = type; msg.x = x; msg.y = y;
    write(fd, &msg, sizeof(msg));

    // Čítame odpoveď
    size_t got = 0;
    while (got < sizeof(stats)) {
        int r = read(fd, ((char*)&stats) + got,
                     sizeof(stats) - got);
        if (r <= 0) {
            break;
        }
        got += r;
    }

    close(fd);
    return stats;
}

// Vlákno: Periodicky číta štatistiky zo servera (~každých 100ms)
void* receiver_thread_func(void* arg)
{
    ClientContext* ctx = (ClientContext*)arg;

    while (ctx->keep_running) {

        pthread_mutex_lock(&ctx->mutex);
        UIState current = ctx->current_state;
        pthread_mutex_unlock(&ctx->mutex);

        // Len v simulácii - inak spime
        if (current == UI_INTERACTIVE || current == UI_SUMMARY) {
            // Požiadame server o aktuálny stav
            StatsMessage new_data =
                send_command(ctx->active_socket_path,
                             MSG_SIM_GET_STATS,
                             0, 0);

            // Skontrolujeme či sú dáta platné
            int valid =
                new_data.width  != 0 &&
                new_data.height != 0;

            if (valid) {
                // Aktualizujeme klientov stav
                pthread_mutex_lock(&ctx->mutex);

                if (ctx->current_state == current) {
                    ctx->stats = new_data;

                    // Ak je simulácia hotová, zastavíme reader
                    if (new_data.finished) {
                        ctx->keep_running = 0;
                        pthread_mutex_unlock(&ctx->mutex);
                        break;
                    }
                }

                pthread_mutex_unlock(&ctx->mutex);
            }

        } else {
            usleep(500000); // Sleep v menu móde
        }

        usleep(100000); // 100ms medzi requestami
    }

    return NULL;
}

// Vlákno: Paralelne číta vstupy od používateľa (v INTERACTIVE/SUMMARY móde)
void* input_thread_func(void* arg) {
    ClientContext* ctx = (ClientContext*)arg;
    
    while (ctx->keep_running) {
        // Len v simulácii sa čítajú vstupy - inak spime
        pthread_mutex_lock(&ctx->mutex);
        UIState current_state = ctx->current_state;
        pthread_mutex_unlock(&ctx->mutex);
        
        if (current_state != UI_INTERACTIVE && current_state != UI_SUMMARY) {
            usleep(50000); // Sleep v menu móde
            continue;
        }
        
        // Non-blocking čítanie vstupov
        int ch = getch();
        
        if (ch == ERR) {
            // Timeout - žiadny vstup
            usleep(10000);
            continue;
        }
        
        // Uložíme vstup do queue
        pthread_mutex_lock(&ctx->input_mutex);
        
        // Ak je queue plná, vyhodíme najstarší
        if (ctx->input_queue_head >= 32) {
            for (int i = 0; i < 31; i++) {
                ctx->input_queue[i] = ctx->input_queue[i + 1];
            }
            ctx->input_queue_head = 31;
        }
        
        ctx->input_queue[ctx->input_queue_head] = ch;
        ctx->input_queue_head++;
        
        pthread_mutex_unlock(&ctx->input_mutex);
    }
    
    return NULL;
}

// Hlavná funkcia klienta - spravuje UI stavy a vlákna
void client_run(void) {
    // Inicializácia ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // Príprava klientovho kontextu - zdieľané medzi vláknami
    ClientContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_mutex_init(&ctx.input_mutex, NULL);
    ctx.keep_running = 1;
    ctx.current_state = UI_MENU_MODE;
    ctx.input_queue_head = 0;

    // Konfiguračné premenné (lokálne v main loopu)
    int mode = 0;
    int x = 5, y = 5, K = 100, runs = 1;
    int probs[4] = {25, 25, 25, 25};
    int height = 11, width = 11;
    char out_filename[128] = {0};

    // Spustenie vlákien
    pthread_t receiver_tid, input_tid;
    pthread_create(&receiver_tid, NULL, receiver_thread_func, &ctx);
    pthread_create(&input_tid, NULL, input_thread_func, &ctx);

    // Hlavná UI slučka
    while (1) {
        // Prečítame aktuálny stav (bezpečne)
        pthread_mutex_lock(&ctx.mutex);
        UIState local_state = ctx.current_state;
        pthread_mutex_unlock(&ctx.mutex);

        switch (local_state) {

        // ===== MENU MODUS =====
        case UI_MENU_MODE: {
            timeout(-1);  // Blokujúce čítanie - čaká na stlačenie
            
            pthread_mutex_lock(&ctx.mutex);
            memset(&ctx.stats, 0, sizeof(ctx.stats));
            pthread_mutex_unlock(&ctx.mutex);

            char room_code[16] = {0};
            int conn_choice = draw_connection_menu(room_code);

            if (conn_choice == 0) break;

            if (conn_choice == 1) {
                // Vytvorenie novej miestnosti
                UIState result = handle_create_new_server(&ctx, room_code, &mode);
                pthread_mutex_lock(&ctx.mutex);
                ctx.current_state = result;
                pthread_mutex_unlock(&ctx.mutex);
            } 
            else if (conn_choice == 2) {
                // Pripojenie k existujúcej miestnosti
                UIState result = handle_connect_to_existing(&ctx);
                pthread_mutex_lock(&ctx.mutex);
                ctx.current_state = result;
                pthread_mutex_unlock(&ctx.mutex);
            }
            break;
        }

        // ===== SETUP MODUS =====
        case UI_SETUP_SIM: {
            timeout(-1);  // Blokujúce čítanie - čaká na vstupy
            
            // Zobrazíme setup menu
            UIState next = draw_setup(
                &x, &y, &K, &runs,
                &width, &height,
                probs, mode,
                out_filename, sizeof(out_filename)
            );

            if (next == UI_SETUP_SIM) {
                break; // Stále editujeme
            }

            if (next == UI_MENU_MODE) {
                // Návrat do menu
                pthread_mutex_lock(&ctx.mutex);
                ctx.current_state = UI_MENU_MODE;
                pthread_mutex_unlock(&ctx.mutex);
                break;
            }

            // Poslanie konfigurácie na server
            int success = send_config_to_server(
                &ctx,
                x, y,
                width, height,
                K, runs,
                probs,
                out_filename,
                &next
            );

            if (!success) {
                // Chyba - vráť do menu
                pthread_mutex_lock(&ctx.mutex);
                ctx.current_state = UI_MENU_MODE;
                pthread_mutex_unlock(&ctx.mutex);
            }

            break;
        }

        // ===== SIMULÁCIA (INTERAKTÍVNY / SUMÁRNY MODUS) =====
        case UI_INTERACTIVE:
        case UI_SUMMARY: {
            timeout(50);  // Non-blocking - čaká na vstupy paralelne
            
            pthread_mutex_lock(&ctx.mutex);
            StatsMessage current_stats = ctx.stats;
            UIState mode_type = ctx.current_state;
            pthread_mutex_unlock(&ctx.mutex);

            if (mode_type == UI_INTERACTIVE) {
                handle_interactive_mode(&ctx, &current_stats, x, y, K, runs);
            } else {
                handle_summary_mode(&ctx, &current_stats, x, y, K, runs);
            }
            break;
        }

        // ===== KONIEC =====
        case UI_EXIT:
            goto cleanup;
        }
    }

cleanup:
    // Zastavenie vlákien
    ctx.keep_running = 0;
    pthread_join(receiver_tid, NULL);
    pthread_join(input_tid, NULL);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_mutex_destroy(&ctx.input_mutex);
    endwin(); // Ukončenie ncurses
}
