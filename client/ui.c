/**
 * ui.c - NCurses UI rendering functions
 * Displays menus, configuration dialogs, and simulation state
 */
#include "../common/common.h"
#include "../common/ipc.h"
#include "../simulation/simulation.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

int draw_connection_menu(char *room_code) {
    clear();
    mvprintw(2, 4, "=== PRIPOJENIE K SIMULACII ===");
    mvprintw(4, 6, "1 - Vytvorit novu miestnost (Server)");
    mvprintw(5, 6, "2 - Pripojit sa k existujucej (Klient)");
    mvprintw(7, 6, "Volba: ");
    refresh();

    timeout(-1); // Blocking input
    int choice = 0;
    do {
        choice = getch();
        if (choice != '1' && choice != '2') {
            choice = 0; // Invalid - retry
        }
    } while (choice == 0);

    // Option 1: ask for room code
    if (choice == '1') {
        move(9, 6);
        clrtoeol();
        printw("Zadaj kod miestnosti (max 15 znakov): ");
        refresh();
        
        echo();
        curs_set(1);
        getnstr(room_code, 15);
        noecho();
        curs_set(0);
    }

    return (choice == '1') ? 1 : 2;
}

UIState draw_mode_menu(int *mode) {
    clear();
    mvprintw(2, 4, "Random Walk Simulation");
    mvprintw(4, 6, "1 - Interaktivny mod");
    mvprintw(5, 6, "2 - Sumarny mod");
    mvprintw(7, 6, "q - Quit");
    refresh();

    timeout(-1);
    int ch = getch();
    
    if (ch == '1') { *mode = 1; return UI_SETUP_SIM; }
    if (ch == '2') { *mode = 2; return UI_SETUP_SIM; }
    if (ch == 'q') return UI_EXIT;
    
    return UI_MENU_MODE;
}
UIState draw_setup(
    int *x, int *y, int *K, int *runs,
    int *width, int *height,
    int probs[4], int mode,
    char *out_filename, int out_filename_len
) {
        enum { F_WIDTH, F_HEIGHT, F_X, F_Y,
            F_UP, F_DOWN, F_LEFT, F_RIGHT,
            F_K, F_RUNS, F_FILENAME, F_COUNT };

    static int field = 0;

    static char buf[F_COUNT][64];
    static int initialized = 0;

    if (!initialized) {
        snprintf(buf[F_WIDTH], 8, "%d", *width);
        snprintf(buf[F_HEIGHT], 8, "%d", *height);
        snprintf(buf[F_X], 8, "%d", *x);
        snprintf(buf[F_Y], 8, "%d", *y);
        snprintf(buf[F_UP], 8, "%d", probs[0]);
        snprintf(buf[F_DOWN], 8, "%d", probs[1]);
        snprintf(buf[F_LEFT], 8, "%d", probs[2]);
        snprintf(buf[F_RIGHT], 8, "%d", probs[3]);
        snprintf(buf[F_K], 8, "%d", *K);
        snprintf(buf[F_RUNS], 8, "%d", *runs);
        snprintf(buf[F_FILENAME], 64, "out.txt");
        initialized = 1;
    }

    clear();
    curs_set(1);
    noecho();

    mvprintw(1, 2, "--- KONFIGURACIA SIMULACIE ---");

    mvprintw(3, 2, "Sirka sveta: %s %s", buf[F_WIDTH], field == F_WIDTH ? "<" : "");
    mvprintw(4, 2, "Vyska sveta: %s %s", buf[F_HEIGHT], field == F_HEIGHT ? "<" : "");
    mvprintw(6, 2, "Start X: %s %s", buf[F_X], field == F_X ? "<" : "");
    mvprintw(7, 2, "Start Y: %s %s", buf[F_Y], field == F_Y ? "<" : "");

    mvprintw(9, 2, "Up: %s %s", buf[F_UP], field == F_UP ? "<" : "");
    mvprintw(10, 2, "Down: %s %s", buf[F_DOWN], field == F_DOWN ? "<" : "");
    mvprintw(11, 2, "Left: %s %s", buf[F_LEFT], field == F_LEFT ? "<" : "");
    mvprintw(12, 2, "Right: %s %s", buf[F_RIGHT], field == F_RIGHT ? "<" : "");

    mvprintw(14, 2, "Max krokov K: %s %s", buf[F_K], field == F_K ? "<" : "");
    if (mode == 2) {
        mvprintw(15, 2, "Replikacie: %s %s", buf[F_RUNS], field == F_RUNS ? "<" : "");
        mvprintw(16, 2, "Vystupny subor: %s %s", buf[F_FILENAME], field == F_FILENAME ? "<" : "");
    } else {
        mvprintw(16, 2, "Vystupny subor: %s %s", buf[F_FILENAME], field == F_FILENAME ? "<" : "");
    }

    mvprintw(17, 2, "ENTER = dalsie | BACKSPACE = mazat | q = spat");

    refresh();
    timeout(50);
    int ch = getch();

    if (ch == 'q' || ch == 27) {
        initialized = 0;
        field = 0;
        return UI_MENU_MODE;
    }

    if (ch == '\n') {
        field++;
        if (field >= F_COUNT) {
            *width = atoi(buf[F_WIDTH]);
            *height = atoi(buf[F_HEIGHT]);
            *x = atoi(buf[F_X]);
            *y = atoi(buf[F_Y]);
            probs[0] = atoi(buf[F_UP]);
            probs[1] = atoi(buf[F_DOWN]);
            probs[2] = atoi(buf[F_LEFT]);
            probs[3] = atoi(buf[F_RIGHT]);
            *K = atoi(buf[F_K]);
            *runs = atoi(buf[F_RUNS]);
            // copy output filename
            if (out_filename && out_filename_len > 0) {
                strncpy(out_filename, buf[F_FILENAME], out_filename_len - 1);
                out_filename[out_filename_len-1] = '\0';
            }

            initialized = 0;
            field = 0;
            return (mode == 1) ? UI_INTERACTIVE : UI_SUMMARY;
        }
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        int len = strlen(buf[field]);
        if (len > 0) buf[field][len - 1] = '\0';
    }

    if (field == F_FILENAME) {
        if (ch >= 32 && ch <= 126) {
            int len = strlen(buf[field]);
            if (len < 63) {
                buf[field][len] = ch;
                buf[field][len + 1] = '\0';
            }
        }
    } else {
        if (ch >= '0' && ch <= '9') {
            int len = strlen(buf[field]);
            if (len < 6) {
                buf[field][len] = ch;
                buf[field][len + 1] = '\0';
            }
        }
    }

    return UI_SETUP_SIM;
}

void draw_world(int height, int width, int posX, int posY, _Bool obstacle[50][50], _Bool visited[50][50]) {
    // Začíname vykresľovať od riadku 3 (aby sme mali miesto pre hlavičku)
    int y_offset = 3;
    
    for (int world_y = 0; world_y < height; world_y++) {
        for (int world_x = 0; world_x < width; world_x++) {
            // Invertujeme y-súradnicu, aby (0,0) bol v ľavom dolnom rohu
            int screen_y = y_offset + (height - 1 - world_y);  // Inverzia + offset
            
            int screen_x = 5 + world_x * 2;  // Offset od ľavého okraja + dvojnásobná šírka
            
            if (posX == world_x && posY == world_y) {
                attron(COLOR_PAIR(1));  // Chodec - červený
                mvprintw(screen_y, screen_x, " @ ");
                attroff(COLOR_PAIR(1));
            } else if (obstacle[world_y][world_x]) {
                attron(COLOR_PAIR(2));  // Prekážka - modrá
                mvprintw(screen_y, screen_x, " # ");
                attroff(COLOR_PAIR(2));
            } else if (visited[world_y][world_x]) {
                attron(COLOR_PAIR(3));  // Navštívené - zelené
                mvprintw(screen_y, screen_x, " . ");
                attroff(COLOR_PAIR(3));
            } else if (world_x == 0 && world_y == 0) {
              mvprintw(screen_y, screen_x, " O ");
            } else {
                mvprintw(screen_y, screen_x, "   ");  // Prázdne - čierne
            }
        }
    }
    
    // Vykreslenie súradnicových osí
    attron(COLOR_PAIR(4));
    
    // Y-ová os (vertikálne čísla)
    for (int world_y = 0; world_y < height; world_y++) {
        int screen_y = y_offset + (height - 1 - world_y);
        mvprintw(screen_y, 1, "%2d ", world_y);  // Čísla y-osi vľavo
    }
    
    // X-ová os (horizontálne čísla)
    mvprintw(y_offset + height, 5, " ");  // Posun pod svet
    for (int world_x = 0; world_x < width; world_x++) {
        mvprintw(y_offset + height, 5 + world_x * 2, "%2d ", world_x);  // Čísla x-osi dole
    }
    
    // Označenie osí
    mvprintw(y_offset - 1, 1, "Y\\X");  // Označenie osí v ľavom hornom rohu
    
    attroff(COLOR_PAIR(4));
}
void draw_summary(double grid[11][11], int w, int h) {
    clear();
    mvprintw(1, 4, "Sumarny mod");

    for (int y = h-1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            mvprintw(3 + (h-1-y), 4 + x*6, "%5.2f", grid[x][y]);
        }
    }
    refresh();
}
void draw_stats(StatsMessage *s , int offset_y , UIState state) {
   
    mvprintw(8 + offset_y, 2, "=== STATISTIKY ===");
    
    if (state == UI_SUMMARY) {
        int total_configured = s->total_runs + s->remaining_runs;
        mvprintw(9 + offset_y, 4, "Total runs: %d / %d", s->total_runs, total_configured);
        mvprintw(10 + offset_y, 4, "Successful runs: %d", s->succ_runs);
        mvprintw(11 + offset_y, 4, "Success rate: %.2f %%", s->success_rate_permille / 10.0f);
        mvprintw(12 + offset_y, 4, "Total steps: %d", s->total_steps);
        
        if (s->total_runs > 0) {
            double avg_steps = (double)s->total_steps / s->total_runs;
            mvprintw(13 + offset_y, 4, "Avg steps/run: %.2f", avg_steps);
        }
        
        if (s->remaining_runs > 0) {
            mvprintw(14 + offset_y, 4, "Remaining: %d", s->remaining_runs);
        }
        
    } else if (state == UI_INTERACTIVE) {
        int visited = 0;
        for (int y = 0; y < s->height; y++) {
            for (int x = 0; x < s->width; x++) {
                if (s->visited[y][x]) visited++;
            }
        }
        
        mvprintw(9 + offset_y, 4, "Steps made: %d / %d", s->curr_steps, s->max_steps);
        mvprintw(10 + offset_y, 4, "Position: (%d, %d)", s->posX, s->posY);
        mvprintw(11 + offset_y, 4, "Visited cells: %d / %d", visited, s->width * s->height);
    }
    
    if (s->finished) {
        mvprintw(1, 2, "*** SIMULACIA UKONCENA ***");
        mvprintw(2, 2, "Stlac 'q' pre vrátenie do menu");
    }

}

int draw_server_list_menu(char *selected_socket_path) {
    cleanup_dead_servers();
    
    int count = 0;
    ServerInfo *servers = list_available_servers(&count);
    
    if (count == 0) {
        mvprintw(10, 4, "Ziadne dostupne servery!");
        mvprintw(11, 4, "Stlac ESC na vrátenie");
        refresh();
        getch();
        return 0;
    }
    
    int selected = 0;
    
    while (1) {
        clear();
        mvprintw(2, 4, "=== DOSTUPNE SERVERY ===");
        mvprintw(3, 4, "(%d servery dostupne)", count);
        
        for (int i = 0; i < count; i++) {
            if (i == selected) {
                attron(A_REVERSE);
            }
            mvprintw(5 + i, 6, "%s (%dx%d)", 
                     servers[i].socket_path, 
                     servers[i].width, 
                     servers[i].height);
            if (i == selected) {
                attroff(A_REVERSE);
            }
        }
        
        mvprintw(6 + count, 4, "UP/DOWN = Vybrat | ENTER = Pridat sa | ESC = Spat");
        refresh();
        
        int ch = getch();
        if (ch == KEY_UP && selected > 0) {
            selected--;
        } else if (ch == KEY_DOWN && selected < count - 1) {
            selected++;
        } else if (ch == '\n') {
            strncpy(selected_socket_path, servers[selected].socket_path, 255);
            free(servers);
            return 1;
        } else if (ch == 27) { // ESC
            free(servers);
            return 0;
        }
    }
}

