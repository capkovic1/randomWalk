#include "menu_handler.h"
#include "ui.h"
#include "../common/common.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

void show_error_dialog(const char *message) {
  erase();
  mvprintw(5, 5, "CHYBA: %s", message);
  mvprintw(7, 5, "Stlac lubovolnu klavesu...");
  refresh();
  getch();
}


int wait_for_server(const char *socket_path, int max_retries) {
  for (int retry = 0; retry < max_retries; retry++) {
  usleep(100000);
        
  struct sockaddr_un test_addr = {0};
  test_addr.sun_family = AF_UNIX;
  strncpy(test_addr.sun_path, socket_path, sizeof(test_addr.sun_path) - 1);
        
  int test_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (test_fd < 0) continue;
        
  if (connect(test_fd, (struct sockaddr *)&test_addr, sizeof(test_addr)) == 0) {
    close(test_fd);
    return 1; 
  }
    close(test_fd);
  }
  return 0; 
}


  int send_config_to_server(ClientContext *ctx,int x, int y,int width, int height,int K, int runs,int *probs,const char *out_filename,double obstacle_ratio,UIState *next_state) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    show_error_dialog("Nie je mozne vytvorit socket!");
    return 0;
  }

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, ctx->active_socket_path, sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    close(fd);
    show_error_dialog("Nie je mozne sa pripojit na server!");
    return 0;
  }

  // Poslanie konfigurácie
  Message configMsg;
  memset(&configMsg, 0, sizeof(configMsg));
  configMsg.type = MSG_SIM_CONFIG;
  configMsg.x = x;
  configMsg.y = y;
  configMsg.width = width;
  configMsg.height = height;
  configMsg.max_steps = K;
  configMsg.replications = runs;
  configMsg.obstacle_ratio = obstacle_ratio;
  memcpy(configMsg.probs, probs, sizeof(configMsg.probs));
  if (out_filename && out_filename[0] != '\0') {
    strncpy(configMsg.out_filename, out_filename, sizeof(configMsg.out_filename) - 1);
    configMsg.out_filename[sizeof(configMsg.out_filename) - 1] = '\0';
  }

  if (write(fd, &configMsg, sizeof(configMsg)) <= 0) {
    close(fd);
    show_error_dialog("Nie je mozne poslat konfiguraciu serveru!");
    return 0;
  }

  // Čítanie odpovede zo servera
  StatsMessage temp_stats = {0};
  size_t got = 0;
  while (got < sizeof(temp_stats)) {
    int r = read(fd, ((char *)&temp_stats) + got, sizeof(temp_stats) - got);
    if (r <= 0) break;
    got += r;
  }

  pthread_mutex_lock(&ctx->mutex);
  ctx->stats = temp_stats;
  ctx->current_state = *next_state;
  pthread_mutex_unlock(&ctx->mutex);

  close(fd);
  return 1; 
}

UIState handle_create_new_server(ClientContext *ctx, char *room_code, int *mode) {
  char socket_path[256] = {0};
  sprintf(socket_path, "/tmp/drunk_%s.sock", room_code);
  strncpy(ctx->active_socket_path, socket_path, sizeof(ctx->active_socket_path) - 1);
  ctx->active_socket_path[sizeof(ctx->active_socket_path) - 1] = '\0';

  UIState mode_result = draw_mode_menu(mode);
  if (mode_result != UI_SETUP_SIM) {
    return UI_MENU_MODE;
  }

  pid_t pid = fork();
  if (pid == 0) {
    setsid(); // Nová session group
    execl("./server_app", "./server_app", socket_path, NULL);
    exit(1); // Ak execl zlyha
  } else if (pid < 0) {
    show_error_dialog("Nie je mozne forknut proces!");
    return UI_MENU_MODE;
  }

  if (!wait_for_server(socket_path, 20)) {
    show_error_dialog("Server sa nestihol spustit (timeout)!");
    return UI_MENU_MODE;
  }

  return UI_SETUP_SIM;
}

UIState handle_connect_to_existing(ClientContext *ctx) {
  char socket_path[256] = {0};

  if (!draw_server_list_menu(socket_path)) {
    return UI_MENU_MODE;
  }
  strncpy(ctx->active_socket_path, socket_path, sizeof(ctx->active_socket_path) - 1);
  ctx->active_socket_path[sizeof(ctx->active_socket_path) - 1] = '\0';

  return UI_INTERACTIVE;
}
