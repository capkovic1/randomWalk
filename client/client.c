#include "client.h"
#include "../common/common.h"

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
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    Message msg = { .type = type, .x = x, .y = y };
    write(fd, &msg, sizeof(msg));

    StatsMessage stats;
    read(fd, &stats, sizeof(stats));
    close(fd);

    return stats;
}


void client_run(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int x = 5, y = 5;
    StatsMessage stats = {0};

    while (1) {
        clear();

        mvprintw(1, 2, "Random Walk Simulation");
        mvprintw(3, 2, "Start position: (%d,%d)", x, y);

        mvprintw(5, 2, "Statistics:");
        mvprintw(6, 4, "Total runs: %d", stats.total_runs);
        mvprintw(7, 4, "Successful runs: %d", stats.succ_runs);
        mvprintw(8, 4, "Total steps: %d", stats.total_steps);

        mvprintw(10, 2, "Controls:");
        mvprintw(11, 4, "r - run simulation");
        mvprintw(12, 4, "c - clear stats");
        mvprintw(13, 4, "q - quit");

        refresh();
        int ch = getch();

        if (ch == 'q') break;
        else if (ch == 'r')
            stats = send_command(MSG_SIM_RUN, x, y);
        else if (ch == 'c')
            stats = send_command(MSG_SIM_RESET, 0, 0);
    }

    endwin();
}

