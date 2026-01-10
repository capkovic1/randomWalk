/**
 * ============================================================================
 * FILE: client/client.c
 * ============================================================================
 * ROLE: Klient aplikácie Random Walk
 * - Spravuje UI cez ncurses
 * - Čita vstupy od používateľa
 * - Komunikuje so serverom cez Unix domain sockety
 * - Spúšťa viacerí vlákna (receiver_thread pre dáta, INPUT_THREAD TODO)
 * 
 * ARCHITEKTÚRA (P2, P7, P11):
 * - Proces: klient_app (fork z main.c)
 * - Vlákna:
 *   1) receiver_thread_func() - čita dáta zo servera (IMPLEMENTOVANÉ)
 *   2) input_thread_func() - TODO - čita vstupy od používateľa (KRITICKÉ)
 *   3) main thread - spravuje UI stavy
 * 
 * IPC (P9, P10):
 * - Type: Unix domain sockets (SOCK_STREAM)
 * - Formát: /tmp/drunk_<room_code>.sock
 * - Registry: /tmp/drunk_servers_registry.txt
 * 
 * ============================================================================
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

/**
 * send_command()
 * Pošle príkaz na server a čaká odpoveď (blocking)
 * 
 * PARAMETRE:
 *   socket_path - cesta k socket-u servera
 *   type - typ správy (MSG_SIM_RUN, MSG_SIM_STEP, atď.)
 *   x, y - súradnice (alebo relevantné dáta)
 * 
 * VRÁTI: StatsMessage s aktuálnym stavom simulácie
 * 
 * PROBLÉM: Táto funkcia je blokujúca - čaká kým server odpovedá
 * Lepší prístup: receiver_thread_func() (IMPLEMENTOVANÉ) vs input_thread (TODO)
 */
StatsMessage send_command(const char* socket_path, MessageType type, int x, int y) {
    StatsMessage stats;
    memset(&stats, 0, sizeof(stats));

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path , sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return stats;
    }

    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = type; msg.x = x; msg.y = y;
    write(fd, &msg, sizeof(msg));

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

/**
 * receiver_thread_func()
 * ============================================================================
 * VLÁKNO: Priebežne čita dáta zo servera (P11)
 * 
 * Táto funkcia bieží v samostatnom vlákne a vykonáva:
 * 1) Okresne periodicky posiela MSG_SIM_GET_STATS na server (~každých 100ms)
 * 2) Prijíma aktualizácie stavu simulácie (počet behu, pozícia walkera, atď.)
 * 3) Aktualizuje ctx->stats (chránené mutexom)
 * 4) Detekuje koniec simulácie a zastavuje sa
 * 
 * SYNCHRONIZÁCIA (P11):
 * - Mutexom chránený ctx->stats - aby main thread mohol bezpečne čítať
 * - ctx->keep_running - signál na zastavenie vlákna
 * 
 * PROBLÉMY V SÚČASNOM KÓDE:
 * - ❌ Blokujúca komunikácia (send_command voláva write() + read())
 * - ⚠️  Vracia sa po skončení (nie infinite loop)
 * - ⚠️  Nespravuje chyby pri komunikácii
 * 
 * TODO (PRIORITA 🔴):
 * - Spustiť skôr receiver ako aby si ich v kóde dát.
 * ============================================================================
 */
void* receiver_thread_func(void* arg)
{
    ClientContext* ctx = (ClientContext*)arg;

    while (ctx->keep_running) {

        pthread_mutex_lock(&ctx->mutex);
        UIState current = ctx->current_state;
        pthread_mutex_unlock(&ctx->mutex);

        if (current == UI_INTERACTIVE || current == UI_SUMMARY) {

            StatsMessage new_data =
                send_command(ctx->active_socket_path,
                             MSG_SIM_GET_STATS,
                             0, 0);

            int valid =
                new_data.width  != 0 &&
                new_data.height != 0;

            if (valid) {

                pthread_mutex_lock(&ctx->mutex);

                if (ctx->current_state == current) {

                    ctx->stats = new_data;

                    if (new_data.finished) {
                        ctx->keep_running = 0; // stop receiver but keep current_state so stats remain shown
                        pthread_mutex_unlock(&ctx->mutex);
                        break;
                    }
                }

                pthread_mutex_unlock(&ctx->mutex);
            }

        } else {
            usleep(500000);
        }

        usleep(100000);
    }

    return NULL;
}

/**
 * input_thread_func()
 * ============================================================================
 * VLÁKNO: Paralelné čítanie vstupov od používateľa (P11)
 * 
 * Táto funkcia bieží v samostatnom vlákne a vykonáva:
 * 1) Non-blocking čítanie pomocou getch() (timeout je nastavený v main)
 * 2) Ukladá vstupy do input_queue v ClientContext (chránené mutexom)
 * 3) Signalizuje ukončenie keď ctx->keep_running == 0
 * 
 * SYNCHRONIZÁCIA (P11):
 * - Mutexom chránená input_queue - aby main thread mohol bezpečne čítať
 * - ctx->keep_running - signál na zastavenie vlákna
 * 
 * VÝHODY TOHTO PRÍSTUPU:
 * ✅ getch() nie je blokujúci - timeout je nastavený v main thread
 * ✅ Main thread a receiver_thread môžu bežať bez zamrzávania
 * ✅ Vstupy sú bufferovania v queue (ak by niekto rýchlo klikal)
 * ✅ Bezpečná synchronizácia cez mutex (nie global variables)
 * 
 * ============================================================================
 */
void* input_thread_func(void* arg) {
    ClientContext* ctx = (ClientContext*)arg;
    
    while (ctx->keep_running) {
        // Čítame vstup len v INTERACTIVE a SUMMARY módoch
        pthread_mutex_lock(&ctx->mutex);
        UIState current_state = ctx->current_state;
        pthread_mutex_unlock(&ctx->mutex);
        
        if (current_state != UI_INTERACTIVE && current_state != UI_SUMMARY) {
            // V menu a setup módoch sleep - vstup sa číta direktne
            usleep(50000); // 50ms pause
            continue;
        }
        
        // Non-blocking čítanie - timeout je nastavený v main (50ms)
        int ch = getch();
        
        // ERR znamená timeout (žiadny vstup)
        if (ch == ERR) {
            usleep(10000); // 10ms - malá pauza aby sme nevyžierali CPU
            continue;
        }
        
        // Vstup je dostupný - uložíme ho do queue
        pthread_mutex_lock(&ctx->input_mutex);
        
        // Ak je queue plná (32 prvkov), vyhodíme najstarší
        if (ctx->input_queue_head >= 32) {
            // Shift vstupov - vyhodíme prvý
            for (int i = 0; i < 31; i++) {
                ctx->input_queue[i] = ctx->input_queue[i + 1];
            }
            ctx->input_queue_head = 31;
        }
        
        // Pridáme nový vstup na koniec queue
        ctx->input_queue[ctx->input_queue_head] = ch;
        ctx->input_queue_head++;
        
        pthread_mutex_unlock(&ctx->input_mutex);
    }
    
    return NULL;
}

/**
 * client_run()
 * ============================================================================
 * HLAVNÁ FUNKCIA KLIENTA (P2, P7, P11)
 * 
 * POPIS:
 * Spravuje hlavnú UI slučku aplikácie - menu a všetky stavy:
 * - UI_MENU_MODE: Výber - Nová sim / Pripojenie / Koniec
 * - UI_SETUP_SIM: Nastavenie parametrov (rozmery, K, replikácie, atď.)
 * - UI_INTERACTIVE: Krok za krokom simulácia
 * - UI_SUMMARY: Hromadný beh všetkých replikácií
 * 
 * ARCHITEKTÚRA VLÁKIEN:
 * 1) main thread - táto funkcia, spravuje UI a menu
 * 2) receiver_thread - číta dáta zo servera (IMPLEMENTOVANÉ ✅)
 * 3) input_thread - číta vstupy od používateľa (IMPLEMENTOVANÉ ✅)
 * 
 * PROBLÉMY V SÚČASNOM KÓDE:
 * ❌ getch() je blokujúci - zamrzáva UI pokým sa čaká na vstup
 * ❌ receiver_thread nesmie bežať paralelne s getch()
 * ⚠️  Synchronizácia módu medzi klientmi (FR6)
 * 
 * KOMENTÁRE K JEDNOTLIVÝM ČIASTIAM KÓDU:
 * - Linka 97-105: Inicializácia ncurses (správne)
 * - Linka 114: Spustenie receiver_thread (SPRÁVNE)
 * - Linka 123-128: Menu - Vytvorenie novej sim (SPRÁVNE)
 * - Linka 130-140: Menu - Pripojenie k sim (SPRÁVNE, ale bez synchronizácie módu)
 * - Linka 242-300: Interaktívny/Sumárny mód (SPRÁVNE logika)
 * 
 * ============================================================================
 */
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
    pthread_mutex_init(&ctx.input_mutex, NULL);  // Inicializácia input mutex (P11)
    ctx.keep_running = 1;
    ctx.current_state = UI_MENU_MODE;
    ctx.input_queue_head = 0;  // Prázdna queue na začiatku

    // Lokálne konfiguračné premenné
    int mode = 0;
    int x = 5, y = 5, K = 100, runs = 1;
    int probs[4] = {25, 25, 25, 25};
    int height = 11, width = 11;
    char out_filename[128] = {0};

    // Spustenie vlákien (P11):
    // 1) receiver_thread - periodicky čita dáta zo servera
    // 2) input_thread - paralelne čita vstupy od používateľa
    pthread_t receiver_tid, input_tid;
    pthread_create(&receiver_tid, NULL, receiver_thread_func, &ctx);
    pthread_create(&input_tid, NULL, input_thread_func, &ctx);

   while (1) {

    pthread_mutex_lock(&ctx.mutex);
    UIState local_state = ctx.current_state;
    pthread_mutex_unlock(&ctx.mutex);

    switch (local_state) {

   // =========================
// MENU
// =========================
case UI_MENU_MODE: {
    
    timeout(-1);  // Blocking mode - čaká na vstup
    
    pthread_mutex_lock(&ctx.mutex);
    memset(&ctx.stats, 0, sizeof(ctx.stats));
    pthread_mutex_unlock(&ctx.mutex);

    char room_code[16] = {0};
    int conn_choice = draw_connection_menu(room_code);

    if (conn_choice == 0) break; // Pouzivatel stlacil nieco ine

    if (conn_choice == 1) {
        // ===== VYTVORIT NOVU MIESTNOST =====
        UIState result = handle_create_new_server(&ctx, room_code, &mode);
        pthread_mutex_lock(&ctx.mutex);
        ctx.current_state = result;
        pthread_mutex_unlock(&ctx.mutex);
    } 
    else if (conn_choice == 2) {
        // ===== PRIPOJIT SA K EXISTUJUCEJ =====
        UIState result = handle_connect_to_existing(&ctx);
        pthread_mutex_lock(&ctx.mutex);
        ctx.current_state = result;
        pthread_mutex_unlock(&ctx.mutex);
    }
    break;
}
    // =========================
    // SETUP
    // =========================
    case UI_SETUP_SIM: {
        timeout(-1);  // Blocking mode pre draw_setup()
        
        UIState next = draw_setup(
            &x, &y, &K, &runs,
            &width, &height,
            probs, mode,
            out_filename, sizeof(out_filename)
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
            // Chyba pri poslaní - vráť do menu
            pthread_mutex_lock(&ctx.mutex);
            ctx.current_state = UI_MENU_MODE;
            pthread_mutex_unlock(&ctx.mutex);
        }
        // Inak sa ctx.current_state už nastavil v send_config_to_server()

        break;
    }

    // =========================
    // INTERACTIVE + SUMMARY
    // =========================
    case UI_INTERACTIVE:
    case UI_SUMMARY: {
        timeout(50);  // Non-blocking mode pre input thread (50ms) (P11)
        
        pthread_mutex_lock(&ctx.mutex);
        StatsMessage current_stats = ctx.stats;
        UIState mode = ctx.current_state;
        pthread_mutex_unlock(&ctx.mutex);

        if (mode == UI_INTERACTIVE) {
            handle_interactive_mode(&ctx, &current_stats, x, y, K, runs);
        } else {
            handle_summary_mode(&ctx, &current_stats, x, y, K, runs);
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

    // Korektné ukončenie vlákien (P11):
    // 1) Signalizujeme obidvom vláknам že majú skončiť
    // 2) Čakáme na ich dokončenie (pthread_join)
    // 3) Vyčisťujeme mutexes a ncurses
    ctx.keep_running = 0;
    pthread_join(receiver_tid, NULL);
    pthread_join(input_tid, NULL);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_mutex_destroy(&ctx.input_mutex);
    endwin();
}
