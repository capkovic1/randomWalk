#include "simulation_handler.h"
#include "ui.h"
#include "../common/common.h"
#include <ncurses.h>
#include <pthread.h>
#include <string.h>
#include "client.h"
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

void handle_interactive_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K, int runs __attribute__((unused))
) {
    static int initialized = 0;
    
    if (!initialized) {
        clear();
        initialized = 1;
    }
    
    // ===== HORNÁ ČASŤ =====
    move(0, 0);
    clrtoeol();
    mvprintw(0, 2, "INTERAKTIVNY MOD | Start:  (%d,%d)", x, y);
    
    move(1, 0);
    clrtoeol();
    mvprintw(1, 2, "r=step  c=reset  q=menu");
    
    // ===== VYKRESLI SVET =====
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
    
    // ===== ŠTATISTIKY (pod svetom) =====
    int stats_y = 3 + world_height;
    
    move(stats_y, 0);
    clrtoeol();
    mvprintw(stats_y, 2, "Krokov: %d / %d", current_stats->curr_steps, K);
    
    move(stats_y + 1, 0);
    clrtoeol();
    mvprintw(stats_y + 1, 2, "Ciel:  [0,0]");
    
    move(stats_y + 2, 0);
    clrtoeol();
    
    refresh();
    timeout(50);
    
    int ch = read_input_from_queue(ctx);
    
    if (ch == 'r') {
        send_command(ctx->active_socket_path, MSG_SIM_STEP, x, y);
    } 
    else if (ch == 'c') {
        initialized = 0;
        StatsMessage reset_response = send_command(ctx->active_socket_path, MSG_SIM_RESET, x, y);
        pthread_mutex_lock(&ctx->mutex);
        ctx->stats = reset_response;
        pthread_mutex_unlock(&ctx->mutex);
    } 
    else if (ch == 'q') {
        initialized = 0;
        pthread_mutex_lock(&ctx->mutex);
        ctx->current_state = UI_MENU_MODE;
        memset(&ctx->stats, 0, sizeof(ctx->stats));
        pthread_mutex_unlock(&ctx->mutex);
    }
}

void handle_summary_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K __attribute__((unused)), int runs __attribute__((unused))
) {
    static int initialized = 0;
    
    if (! initialized) {
        clear();
        initialized = 1;
    }
    
    // ===== HORNÁ ČASŤ =====
    move(0, 0);
    clrtoeol();
    mvprintw(0, 2, "SUMARNY MOD | Start: (%d,%d)", x, y);
    
    move(1, 0);
    clrtoeol();
    mvprintw(1, 2, "r=start  c=reset  q=menu");
    
    // ===== ŠTATISTIKY =====
    move(3, 0);
    clrtoeol();
    mvprintw(3, 2, "Celk. behy: %d", current_stats->total_runs);
    
    move(4, 0);
    clrtoeol();
    mvprintw(4, 2, "Uspechy: %d", current_stats->succ_runs);
    
    move(5, 0);
    clrtoeol();
    mvprintw(5, 2, "Uspesnost: %.2f%%", current_stats->success_rate_permille / 10.0f);
    
    move(6, 0);
    clrtoeol();
    mvprintw(6, 2, "Priemerne krokov: %.2f", 
             current_stats->total_runs > 0 ? (double)current_stats->total_steps / current_stats->total_runs : 0);
    
    move(7, 0);
    clrtoeol();
    
    refresh();
    timeout(50);
    
    int ch = read_input_from_queue(ctx);
    
    if (ch == 'r') {
        send_command(ctx->active_socket_path, MSG_SIM_RUN, x, y);
    } 
    else if (ch == 'c') {
        initialized = 0;
        send_command(ctx->active_socket_path, MSG_SIM_RESET, x, y);
    } 
    else if (ch == 'q') {
        initialized = 0;
        pthread_mutex_lock(&ctx->mutex);
        ctx->current_state = UI_MENU_MODE;
        memset(&ctx->stats, 0, sizeof(ctx->stats));
        pthread_mutex_unlock(&ctx->mutex);
    }
}
