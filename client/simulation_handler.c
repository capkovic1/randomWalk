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
        erase();
        initialized = 1;
    }
    
    // Vykresli bez problémov - iba mvprintw, bez move()
    mvprintw(0, 0, "========================================");
    mvprintw(1, 0, "  INTERAKTIVNY MOD - RANDOM WALK     ");
    mvprintw(2, 0, "========================================");
    
    mvprintw(3, 0, "Start: [%d,%d]  |  Ciel: [0,0]       ", x, y);
    mvprintw(4, 0, "Klávesy: [r]=step  [c]=reset  [q]=menu");
    mvprintw(5, 0, "");
    
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
    // Riadky 0-5: header (6 riadkov)
    // Riadky 5-17: grid s top+world_height+bottom
    // Riadky 19+: štatistiky
    int stats_y = 6 + 1 + world_height + 1;
    
    mvprintw(stats_y, 0, "[--- STATISTIKY ---]                 ");
    mvprintw(stats_y + 1, 0, "Kroky:       %3d / %d             ", current_stats->curr_steps, K);
    mvprintw(stats_y + 2, 0, "Pozicia:     [%d, %d]              ", current_stats->posX, current_stats->posY);
    
    int visited_count = 0;
    for (int y_i = 0; y_i < world_height; y_i++) {
        for (int x_i = 0; x_i < world_width; x_i++) {
            if (current_stats->visited[y_i][x_i]) visited_count++;
        }
    }
    mvprintw(stats_y + 3, 0, "Navstivene:  %d / %d buniek        ", visited_count, world_width * world_height);
    mvprintw(stats_y + 4, 0, "[-------------------]               ");
    
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
        erase();
        initialized = 1;
    }
    
    // Vykresli bez problémov - iba mvprintw, bez move()
    mvprintw(0, 0, "========================================");
    mvprintw(1, 0, "  SUMARNY MOD - BATCH SIMULATION     ");
    mvprintw(2, 0, "========================================");
    
    mvprintw(3, 0, "Start: [%d,%d]  |  Ciel: [0,0]       ", x, y);
    mvprintw(4, 0, "Klávesy: [r]=spustit  [c]=reset  [q]=menu");
    mvprintw(5, 0, "");
    
    // ===== ŠTATISTIKY =====
    mvprintw(7, 0, "[--- VYSLEDKY ---]                  ");
    mvprintw(8, 0, "Celkove behy:    %d behov           ", current_stats->total_runs);
    mvprintw(9, 0, "Uspesne behy:    %d behov           ", current_stats->succ_runs);
    mvprintw(10, 0, "Uspesnost:       %.2f %%            ", current_stats->success_rate_permille / 10.0f);
    
    if (current_stats->total_runs > 0) {
        double avg_steps = (double)current_stats->total_steps / current_stats->total_runs;
        mvprintw(11, 0, "Priemer krokov:  %.2f krokov/beh   ", avg_steps);
    } else {
        mvprintw(11, 0, "Priemer krokov:  - (ziadny beh)    ");
    }
    
    mvprintw(12, 0, "Zostavajuce:     %d behov           ", current_stats->remaining_runs);
    mvprintw(13, 0, "[-------------------]               ");
    
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

