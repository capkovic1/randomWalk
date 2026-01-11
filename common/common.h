#pragma once

#define SOCKET_PATH "/tmp/random_walk.sock"

#include <pthread.h>

typedef enum { 
  MSG_SIM_RUN = 1,
  MSG_SIM_RESET = 2,
  MSG_SIM_GET_STATS =3,
  MSG_SIM_INIT,
  MSG_SIM_STEP,
  MSG_SIM_CONFIG
}MessageType;

typedef struct {
  MessageType type;
  int x;
  int y;
  int width;
  int height;
  int max_steps;
  int replications;
  int probs[4];
  char out_filename[128];

} Message;
typedef struct {
  int total_steps;
  int max_steps;
  int succ_runs;
  int total_runs;
  
  int width;
  int height;
  _Bool obstacle[50][50];
  _Bool visited[50][50];
  
  int posX;
  int posY;
  
  int curr_steps;
  
  _Bool finished;
  
  int success_rate_permille;
  int remaining_runs;
} StatsMessage;

typedef enum {
  UI_MENU_MODE,
  UI_SETUP_SIM,
  UI_INTERACTIVE,
  UI_SUMMARY,
  UI_EXIT
} UIState;

typedef struct {
  StatsMessage stats;           
  pthread_mutex_t mutex;        
  int server_fd;                
  int keep_running;           
  UIState current_state;        
  char active_socket_path[100]; 
    
  pthread_mutex_t input_mutex;  
  int input_char;             
  int input_queue[32];          
  int input_queue_head;         
} ClientContext;


