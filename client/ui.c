#include "../common/common.h"
#include "../simulation/simulation.h"
#include <ncurses.h>

UIState draw_mode_menu(int *mode) {
    clear();
    mvprintw(2, 4, "Random Walk Simulation");
    mvprintw(4, 6, "1 - Interaktivny mod");
    mvprintw(5, 6, "2 - Sumarny mod");
    mvprintw(7, 6, "q - Quit");
    refresh();

    int ch = getch();
    if (ch == '1') { *mode = 1; return UI_SETUP_SIM; }
    if (ch == '2') { *mode = 2; return UI_SETUP_SIM; }
    if (ch == 'q') return UI_EXIT;
    return UI_MENU_MODE;
}
void draw_setup(int *x, int *y, int *K, int *runs, int *width, int *height, int probs[4], int mode) {
    clear();
    echo();      // Zapne zobrazovanie písaných znakov
    curs_set(1); // Zobrazí kurzor

    mvprintw(1, 2, "--- KONFIGURACIA SIMULACIE ---");

    // 1. Rozmery sveta
    mvprintw(3, 2, "Sirka sveta (1-50): ");
    scanw("%d", width);
    mvprintw(4, 2, "Vyska sveta (1-50): ");
    scanw("%d", height);

    // 2. Startovacia pozicia
    mvprintw(6, 2, "Startovacia pozicia X: ");
    scanw("%d", x);
    mvprintw(7, 2, "Startovacia pozicia Y: ");
    scanw("%d", y);

    // 3. Pravdepodobnosti
    mvprintw(9, 2, "Pravdepodobnosti (sucet musi byt 100):");
    mvprintw(10, 4, "Hore (Up): ");    scanw("%d", &probs[0]);
    mvprintw(11, 4, "Dole (Down): ");  scanw("%d", &probs[1]);
    mvprintw(12, 4, "Vlavo (Left): "); scanw("%d", &probs[2]);
    mvprintw(13, 4, "Vpravo (Right): "); scanw("%d", &probs[3]);

    // 4. Parametre simulacie
    mvprintw(15, 2, "Max krokov (K): ");
    scanw("%d", K);

    if (mode == 2) { // Ak je zvoleny SUMMARY MOD (predpokladam podla tvojho menu)
        mvprintw(16, 2, "Pocet replikacii (N): ");
        scanw("%d", runs);
    } else {
        *runs = 1;
    }

    noecho();     // Vypne spatne echo
    curs_set(0);  // Schova kurzor
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
      mvprintw(10 + offset_y, 4, "reached end : %d",s->succ_runs);
    }
}





