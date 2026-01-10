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
 */
void handle_interactive_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K __attribute__((unused)), int runs __attribute__((unused))
) {
    clear();
    
    mvprintw(1, 2, "INTERAKTIVNY MOD | Start: (%d,%d)", x, y);
    mvprintw(2, 2, "r - krok, c - reset, q - menu");
    
    draw_world(
        current_stats->height,
        current_stats->width,
        current_stats->posX,
        current_stats->posY,
        current_stats->obstacle,
        current_stats->visited
    );
    
    draw_stats(current_stats, 3 + current_stats->height + 2, UI_INTERACTIVE);
    
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
    
    draw_stats(current_stats, 5, UI_SUMMARY);
    
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
