#include "client.h"
#include "../common/common.h"
#include "ui.h"
#include "menu_handler.h"
#include "simulation_handler.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include <term.h>


StatsMessage send_command(const char* socket_path, MessageType type, int x, int y) {
  StatsMessage stats;
  memset(&stats, 0, sizeof(stats));

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return stats;
  }
  
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path , sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    return stats; 
  }

  Message msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = type; msg.x = x; msg.y = y;
  
  if (write(fd, &msg, sizeof(msg)) < 0) {
    close(fd);
    return stats;
  }

  size_t got = 0;
  while (got < sizeof(stats)) {
    int r = read(fd, ((char*)&stats) + got, sizeof(stats) - got);
    if (r <= 0) {
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

    pthread_mutex_lock(&ctx->mutex);
    UIState current = ctx->current_state;
    pthread_mutex_unlock(&ctx->mutex);

    if (current == UI_INTERACTIVE || current == UI_SUMMARY) {

      StatsMessage new_data = send_command(ctx->active_socket_path,MSG_SIM_GET_STATS,0, 0);
      int valid = new_data.width  != 0 && new_data.height != 0;

      if (valid) {
        pthread_mutex_lock(&ctx->mutex);

        if (ctx->current_state == current) {
          ctx->stats = new_data;
          if (new_data.finished) {
          ctx->keep_running = 0;
          pthread_mutex_unlock(&ctx->mutex);
          break;
          }
        }

        pthread_mutex_unlock(&ctx->mutex);
      }

    } else {
      usleep(500000); 
    }
      usleep(100000); 
    }

    return NULL;
}

void* input_thread_func(void* arg) {
  ClientContext* ctx = (ClientContext*)arg;
    
  while (ctx->keep_running) {
    pthread_mutex_lock(&ctx->mutex);
    UIState current_state = ctx->current_state;
    pthread_mutex_unlock(&ctx->mutex);
        
    if (current_state != UI_INTERACTIVE && current_state != UI_SUMMARY) {
      usleep(50000);
      continue;
    }
        
    int ch = getch();
        
    if (ch == ERR) {
      usleep(10000);
      continue;
    }

    pthread_mutex_lock(&ctx->input_mutex);
        
    if (ctx->input_queue_head >= 32) {
      for (int i = 0; i < 31; i++) {
        ctx->input_queue[i] = ctx->input_queue[i + 1];
      }
      ctx->input_queue_head = 31;
    }
        
    ctx->input_queue[ctx->input_queue_head] = ch;
    ctx->input_queue_head++;
        
    pthread_mutex_unlock(&ctx->input_mutex);
  }
    
  return NULL;
}

void client_run(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  ClientContext ctx;
  memset(&ctx, 0, sizeof(ctx));
  pthread_mutex_init(&ctx.mutex, NULL);
  pthread_mutex_init(&ctx.input_mutex, NULL);
  ctx.keep_running = 1;
  ctx.current_state = UI_MENU_MODE;
  ctx.input_queue_head = 0;

  int mode = 0;
  int x = 5, y = 5, K = 100, runs = 1;
  int probs[4] = {25, 25, 25, 25};
  int height = 11, width = 11;
  char out_filename[128] = {0};
  double obstacle_ratio = 0.0;

  pthread_t receiver_tid, input_tid;
  pthread_create(&receiver_tid, NULL, receiver_thread_func, &ctx);
  pthread_create(&input_tid, NULL, input_thread_func, &ctx);

  while (1) {
    pthread_mutex_lock(&ctx.mutex);
    UIState local_state = ctx.current_state;
    pthread_mutex_unlock(&ctx.mutex);

    switch (local_state) {

    case UI_MENU_MODE: {
      timeout(-1);  
            
      pthread_mutex_lock(&ctx.mutex);
      memset(&ctx.stats, 0, sizeof(ctx.stats));
      pthread_mutex_unlock(&ctx.mutex);

      char room_code[16] = {0};
      int conn_choice = draw_connection_menu(room_code);

      if (conn_choice == 0) break;

      if (conn_choice == 1) {
        UIState result = handle_create_new_server(&ctx, room_code, &mode);
        pthread_mutex_lock(&ctx.mutex);
        ctx.current_state = result;
        pthread_mutex_unlock(&ctx.mutex);
      } else if (conn_choice == 2) {
        UIState result = handle_connect_to_existing(&ctx);
        pthread_mutex_lock(&ctx.mutex);
        ctx.current_state = result;
        pthread_mutex_unlock(&ctx.mutex);
      } else if (conn_choice == 3) {
        goto cleanup;
      }
      break;
    }

    case UI_SETUP_SIM: {
      timeout(-1);  
      UIState next = draw_setup(&x, &y, &K, &runs,&width, &height,probs, mode,out_filename, sizeof(out_filename), &obstacle_ratio);

      if (next == UI_SETUP_SIM) {
        break; 
      }

      if (next == UI_MENU_MODE) {
        pthread_mutex_lock(&ctx.mutex);
        ctx.current_state = UI_MENU_MODE;
        pthread_mutex_unlock(&ctx.mutex);
        break;
      }

      int success = send_config_to_server(&ctx,x, y,width, height,K, runs,probs,out_filename,obstacle_ratio,&next);

      if (!success) {
        pthread_mutex_lock(&ctx.mutex);
        ctx.current_state = UI_MENU_MODE;
        pthread_mutex_unlock(&ctx.mutex);
      }

      break;
    }
    case UI_INTERACTIVE:
    case UI_SUMMARY: {
      timeout(50);  
            
      pthread_mutex_lock(&ctx.mutex);
      StatsMessage current_stats = ctx.stats;
      UIState mode_type = ctx.current_state;
      pthread_mutex_unlock(&ctx.mutex);

      if (mode_type == UI_INTERACTIVE) {
        handle_interactive_mode(&ctx, &current_stats, x, y, K, runs);
      } else {
        handle_summary_mode(&ctx, &current_stats, x, y, K, runs);
      }
      break;
    }

    case UI_EXIT:
      goto cleanup;
    }
  }

  cleanup:
    ctx.keep_running = 0;
    pthread_join(receiver_tid, NULL);
    pthread_join(input_tid, NULL);
    pthread_mutex_destroy(&ctx.mutex);
    pthread_mutex_destroy(&ctx.input_mutex);
    
    // Aggressive ncurses cleanup
    endwin();
    if (cur_term != NULL) {
        del_curterm(cur_term);
    }
    
    // Exit with cleanup - kernel will free all memory
    exit(0);
}
