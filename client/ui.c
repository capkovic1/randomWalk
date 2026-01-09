#include "../common/common.h"
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

    int choice = getch();
    if (choice != '1' && choice != '2') return 0; // Neplatná voľba

    // Zadanie kódu miestnosti
    move(9, 6);
    printw("Zadaj kod miestnosti (max 15 znakov): ");
    
    echo();             // Zapneme zobrazovanie pisanych znakov
    curs_set(1);        // Ukazeme kurzor
    getnstr(room_code, 15); // Bezpecne nacitame retazec
    noecho();           // Vypneme echo
    curs_set(0);        // Schovame kurzor

    return (choice == '1') ? 1 : 2;
}

UIState draw_mode_menu(int *mode) {
    clear();
    mvprintw(2, 4, "Random Walk Simulation");
    mvprintw(4, 6, "1 - Interaktivny mod");
    mvprintw(5, 6, "2 - Sumarny mod");
    mvprintw(7, 6, "q - Quit\n");
    refresh();

    int ch = getch();
    if (ch == '1') { *mode = 1; return UI_SETUP_SIM; }
    if (ch == '2') { *mode = 2; return UI_SETUP_SIM; }
    if (ch == 'q') return UI_EXIT;
    return UI_MENU_MODE;
}
UIState draw_setup(
    int *x, int *y, int *K, int *runs,
    int *width, int *height,
    int probs[4], int mode
) {
    enum { F_WIDTH, F_HEIGHT, F_X, F_Y,
           F_UP, F_DOWN, F_LEFT, F_RIGHT,
           F_K, F_RUNS, F_COUNT };

    static int field = 0;

    static char buf[F_COUNT][8];
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
        if (field >= (mode == 2 ? F_COUNT : F_RUNS)) {
            *width = atoi(buf[F_WIDTH]);
            *height = atoi(buf[F_HEIGHT]);
            *x = atoi(buf[F_X]);
            *y = atoi(buf[F_Y]);
            probs[0] = atoi(buf[F_UP]);
            probs[1] = atoi(buf[F_DOWN]);
            probs[2] = atoi(buf[F_LEFT]);
            probs[3] = atoi(buf[F_RIGHT]);
            *K = atoi(buf[F_K]);
            if (mode == 2) *runs = atoi(buf[F_RUNS]);
            else *runs = 1;

            initialized = 0;
            field = 0;
            return (mode == 1) ? UI_INTERACTIVE : UI_SUMMARY;
        }
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        int len = strlen(buf[field]);
        if (len > 0) buf[field][len - 1] = '\0';
    }

    if (ch >= '0' && ch <= '9') {
        int len = strlen(buf[field]);
        if (len < 6) {
            buf[field][len] = ch;
            buf[field][len + 1] = '\0';
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
   
    mvprintw(8 + offset_y, 2, "STATISTIKY:");
    if (state == UI_SUMMARY ) {
    
    
      mvprintw(9 + offset_y, 4, "Total runs: %d", s->total_runs);
      mvprintw(10 + offset_y, 4, "Successful runs: %d", s->succ_runs);

      if (s->total_runs > 0) {
        double p = 100.0 * s->succ_runs / s->total_runs;
        mvprintw(11 + offset_y, 4, "Success rate: %.2f %%", p);
      }

      mvprintw(12 + offset_y, 4, "Total steps: %d", s->total_steps);
    } else if (state == UI_INTERACTIVE ) {
      mvprintw(9+ offset_y, 4, "Steps made currentli: %d" , s->curr_steps);
      mvprintw(10 + offset_y, 4, "reached end : %d",s->finished);
    }
  if (s->finished) {
    mvprintw(0, 2, "SIMULACIA UKONCENA");
    timeout(-1); 
  }

}





