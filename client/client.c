#include "client.h"
#include "../common/common.h"
#include "ui.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>

static void send_sim_run(int x, int y) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    Message msg = {
        .type = MSG_SIM_RUN,
        .x = x,
        .y = y
    };

    write(fd, &msg, sizeof(msg));
    close(fd);
}
static StatsMessage send_command(MessageType type, int x, int y) {
    StatsMessage stats;
    memset(&stats, 0, sizeof(stats));   // 👈 VEĽMI DÔLEŽITÉ

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

    // 👇 SPRÁVNE ČÍTANIE CELÉHO STRUCTU
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


void client_run(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    UIState state = UI_MENU_MODE;
    int mode = 0;
    int x = 5, y = 5, K = 100, runs = 1;

    StatsMessage stats = {0};

    while (state != UI_EXIT) {
        switch (state) {

        // ================= MENU =================
        case UI_MENU_MODE:
            state = draw_mode_menu(&mode);
            break;

        // ================= SETUP =================
        case UI_SETUP_SIM:
            draw_setup(&x, &y, &K, &runs, mode);
            state = (mode == 1) ? UI_INTERACTIVE : UI_SUMMARY;
            break;

        // ================= INTERACTIVE =================
        case UI_INTERACTIVE:
          clear();
          mvprintw(1, 2, "INTERAKTIVNY MOD");
          mvprintw(14, 2, "Start: (%d,%d)", x, y);
          mvprintw(15, 2, "r - run, c - reset, q - back");
          
          if(stats.height == 0 && stats.width == 0 ) {
            stats = send_command(MSG_SIM_INIT, 5, 5);


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
            stats = send_command(MSG_SIM_RESET, 0, 0);
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
              stats = send_command(MSG_SIM_INIT, 5, 5);


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
                stats = send_command(MSG_SIM_RESET, 0, 0);
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

