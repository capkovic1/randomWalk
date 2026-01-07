#pragma once

#define SOCKET_PATH "/tmp/random_walk.sock"

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

} Message;
typedef struct {
  int total_steps;
  int max_steps;
  int succ_runs;
  int total_runs;
  int width ;
  int height;
  _Bool obstacle[50][50];
  _Bool visited[50][50];
  int posX;
  int posY;
  int curr_steps;
} StatsMessage;

typedef enum {
  UI_MENU_MODE,
  UI_SETUP_SIM,
  UI_INTERACTIVE,
  UI_SUMMARY,
  UI_EXIT
} UIState;


