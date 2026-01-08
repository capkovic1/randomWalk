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
        // V reálnom scenári by si tu mal trvalé spojenie alebo 
        // by si sa periodicky pýtal na stav (polling)
        if (ctx->current_state == UI_INTERACTIVE || ctx->current_state == UI_SUMMARY) {
            // Tu zavoláš upravený send_command, ktorý len načíta dáta
            StatsMessage new_data = send_command(MSG_SIM_INIT, 0, 0); 
            
            pthread_mutex_lock(&ctx->mutex);
            ctx->stats = new_data;
            pthread_mutex_unlock(&ctx->mutex);
        }
        usleep(100000); // Obnova každých 100ms
    }
    return NULL;
}

void client_run(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

  UIState state = UI_MENU_MODE;
  
  int mode = 0;
  int x = 5 ;
  int y = 5;
  int K = 100;
  int runs = 1;
  int probs[4] = {25,25,25,25};
  int height = 11;
  int width = 11;
  
    

  StatsMessage stats = {0};

  while (state != UI_EXIT) {
    switch (state) {

       // ================= MENU =================
case UI_MENU_MODE:
    memset(&stats, 0, sizeof(stats));
    state = draw_mode_menu(&mode);
    
    // P3: Ak používateľ vybral novú simuláciu, vytvoríme proces server
    if (state == UI_SETUP_SIM) {
        pid_t pid = fork();
        if (pid < 0) {
            // Chyba pri forku
            endwin();
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // --- DETSKÝ PROCES (SERVER) ---
            // P5: Odpojíme od terminálu, aby server žil aj po vypnutí klienta
            setsid(); 
            
            // Spustíme binárku servera (predpokladá sa názov súboru "server")
            execl("./server_app", "./server_app", NULL);
            
            // Ak exec zlyhá
            exit(1); 
        }
        // --- RODIČOVSKÝ PROCES (KLIENT) ---
        // Pokračuje ďalej v case UI_SETUP_SIM
        usleep(100000); // Malá pauza, aby server stihol vytvoriť socket
    }
    break;
        // ================= SETUP =================
  case UI_SETUP_SIM:
    // Zavolame novu metodu (odovzdavame adresy premennych cez &)
    draw_setup(&x, &y, &K, &runs, &width, &height, probs, mode);
    int originalx = x;
    int originaly = y;  
    // Po ukonceni setupu odosleme specialnu Message na server
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      Message configMsg = {
        .type = MSG_SIM_CONFIG, // Musis pridat do common.h
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .max_steps = K,
        .replications = runs
      };
        // Skopirujeme pole pravdepodobnosti
      memcpy(configMsg.probs, probs, sizeof(probs));

      write(fd, &configMsg, sizeof(configMsg));
        
      // Pockame na potvrdenie (StatsMessage), aby sa nam inicializovali 'stats' v klientovi
      int got = 0;
      while (got < sizeof(stats)) {
        int r = read(fd, ((char*)&stats) + got, sizeof(stats) - got);
        if (r <= 0) break;
        got += r;
      }
      close(fd);
    }

    // Prepneme stav podla modu
    state = (mode == 1) ? UI_INTERACTIVE : UI_SUMMARY;
    break;
      // ================= INTERACTIVE =================
      case UI_INTERACTIVE:
        clear();
        mvprintw(1, 2, "INTERAKTIVNY MOD");
        mvprintw(14, 2, "Start: (%d,%d)", x, y);
        mvprintw(15, 2, "r - run, c - reset, q - back");
          
        if(stats.height == 0 && stats.width == 0 ) {
          stats = send_command(MSG_SIM_INIT, x, y);


          if (stats.width == 0 || stats.height == 0) {
            printf("ERROR: invalid start position\n");
            return;
          }

        }

        draw_world(stats.height, stats.width, stats.posX, stats.posY, stats.obstacle, stats.visited);
          

        int stats_y = 3 + stats.height + 2;


        draw_stats(&stats, stats_y , state);
        refresh();
        int ch = getch();

        if (ch == 'r') {
          
          stats = send_command(MSG_SIM_STEP, x, y);
        
        } else if (ch == 'c') {
            
          stats = send_command(MSG_SIM_RESET, x, y);
        
        } else if (ch == 'q') {
        
          state = UI_MENU_MODE;
        
        }
          break;

        // ================= SUMMARY =================
        case UI_SUMMARY:
            clear();
            mvprintw(1, 2, "SUMARNY MOD");
            mvprintw(3, 2, "Start: (%d,%d), K=%d, runs=%d", x, y, K, runs);
            mvprintw(5, 2, "r - run N, c - reset, q - back");
            draw_stats(&stats, 5 , state);
            if(stats.height == 0 && stats.width == 0 ) {
              stats = send_command(MSG_SIM_INIT, x, y);


             if (stats.width == 0 && stats.height == 0) {
                printf("ERROR: invalid start position\n");
                return;
              }

          }

            refresh();
            ch = getch();

            if (ch == 'r') {
              stats = send_command(MSG_SIM_RUN, x, y);
            } else if (ch == 'c') {
                stats = send_command(MSG_SIM_RESET, x, y);
            } else if (ch == 'q') {
                state = UI_MENU_MODE;
            }
            break;

        default:
            state = UI_EXIT;
        }
    }

    endwin();
}

