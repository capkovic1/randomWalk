#include "simulation_handler.h"
#include "ui.h"
#include "../common/common.h"
#include <ncurses.h>
#include <pthread.h>
#include <string.h>

/**
 * Čítanie vstupov z thread-safe queue
 * Vracia: vstupný znak (0 ak queue je prázdna)
 */
int read_input_from_queue(ClientContext *ctx) {
    int ch = 0;
    pthread_mutex_lock(&ctx->input_mutex);
    if (ctx->input_queue_head > 0) {
        // Vezmeme prvý vstup z queue
        ch = ctx->input_queue[0];
        // Shift všetkých ostatných vstupov doľava
        for (int i = 0; i < ctx->input_queue_head - 1; i++) {
            ctx->input_queue[i] = ctx->input_queue[i + 1];
        }
        ctx->input_queue_head--;
    }
    pthread_mutex_unlock(&ctx->input_mutex);
    return ch;
}

/**
 * Handle interaktívny mód
 * - Zobrazí svet a štatistiky
 * - Čaká na vstup: r (krok), c (reset), q (menu)
 * - Zobrazí aj globálne štatistiky zo servera (ak sú iní klienti)
 */
void handle_interactive_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K __attribute__((unused)), int runs __attribute__((unused))
) {
    clear();
    
    // Horná časť - interaktívny mód
    mvprintw(1, 2, "INTERAKTIVNY MOD | Start: (%d,%d)", x, y);
    mvprintw(2, 2, "r - krok, c - reset, q - menu");
    
    // Vykresliť svet
    int world_height = current_stats->height;
    int world_width = current_stats->width;
    draw_world(
        world_height,
        world_width,
        current_stats->posX,
        current_stats->posY,
        current_stats->obstacle,
        current_stats->visited
    );
    
    // Spodná časť - globálne štatistiky (z sumárneho módu iných klientov)
    int stats_y = 3 + world_height + 2;
    
    mvprintw(stats_y, 2, "=== GLOBALNE STATISTIKY (ine klienty) ===");
    mvprintw(stats_y + 1, 2, "Celk. behy: %d | Uspechy: %d (%.1f%%)",
        current_stats->total_runs,
        current_stats->succ_runs,
        current_stats->total_runs > 0 ? (current_stats->success_rate_permille / 10.0) : 0.0
    );
    mvprintw(stats_y + 2, 2, "Celk. krokov: %d | Zvysajuci: %d",
        current_stats->total_steps,
        current_stats->remaining_runs
    );
    
    // Ak je simulacia hotova
    if (current_stats->finished) {
        mvprintw(stats_y + 3, 2, "[SIMULACIA SKONCENA]");
    }
    
    refresh();
    timeout(50); // Non-blocking getch
    
    int ch = read_input_from_queue(ctx);
    
    if (ch == 'r') {
        // Jeden krok walkera
        send_command(ctx->active_socket_path, MSG_SIM_STEP, x, y);
    } 
    else if (ch == 'c') {
        // Reset simulácie
        send_command(ctx->active_socket_path, MSG_SIM_RESET, x, y);
    } 
    else if (ch == 'q') {
        // Návrat do menu
        pthread_mutex_lock(&ctx->mutex);
        ctx->current_state = UI_MENU_MODE;
        memset(&ctx->stats, 0, sizeof(ctx->stats));
        pthread_mutex_unlock(&ctx->mutex);
    }
}

/**
 * Handle sumárny mód
 * - Spúšťa N replikácií na serveri
 * - Zobrazí agregované štatistiky
 * - Zobrazí aj krok-za-krokom progres ak sú iní klienti v interaktívnom móde
 * - Čaká na vstup: r (spustiť), c (reset), q (menu)
 */
void handle_summary_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K, int runs
) {
    clear();
    
    mvprintw(1, 2, "SUMARNY MOD | K=%d, replikacie=%d", K, runs);
    mvprintw(2, 2, "r - spustit, c - reset, q - menu");
    
    // Sumárne štatistiky
    mvprintw(4, 2, "=== STATISTIKY REPLIKACII ===");
    mvprintw(5, 2, "Celk. behy: %d/%d | Uspechy: %d",
        current_stats->total_runs,
        runs,
        current_stats->succ_runs
    );
    mvprintw(6, 2, "Uspecnost: %.1f%% | Priemer krokov: %.1f",
        current_stats->total_runs > 0 ? (current_stats->success_rate_permille / 10.0) : 0.0,
        current_stats->total_runs > 0 ? ((float)current_stats->total_steps / current_stats->total_runs) : 0.0
    );
    
    // Indikátor z iných klientov (ak sú v interaktívnom móde)
    if (current_stats->posX != x || current_stats->posY != y || current_stats->curr_steps > 0) {
        mvprintw(8, 2, "=== INTERAKTIVNY PROGRESS (INIE KLIENTY) ===");
        mvprintw(9, 2, "Pozicia: (%d,%d) | Krokov: %d/%d",
            current_stats->posX,
            current_stats->posY,
            current_stats->curr_steps,
            K
        );
    }
    
    // V summary móde: ak je finished alebo remaining_runs == 0, automaticky reset
    if (current_stats->finished || current_stats->remaining_runs == 0) {
        if (current_stats->total_runs > 0) {
            send_command(ctx->active_socket_path, MSG_SIM_RESET, x, y);
        }
    }
    
    refresh();
    timeout(50); // Non-blocking getch
    
    int ch = read_input_from_queue(ctx);
    
    if (ch == 'r') {
        // Spustiť simulácie (sumárny mód)
        send_command(ctx->active_socket_path, MSG_SIM_RUN, x, y);
    } 
    else if (ch == 'c') {
        // Reset simulácie
        send_command(ctx->active_socket_path, MSG_SIM_RESET, x, y);
    } 
    else if (ch == 'q') {
        // Návrat do menu
        pthread_mutex_lock(&ctx->mutex);
        ctx->current_state = UI_MENU_MODE;
        memset(&ctx->stats, 0, sizeof(ctx->stats));
        pthread_mutex_unlock(&ctx->mutex);
    }
}
