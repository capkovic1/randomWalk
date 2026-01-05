#pragma once

#define SOCKET_PATH "/tmp/random_walk.sock"

typedef enum {
    MSG_SIM_RUN = 1,
    MSG_SIM_RESET = 2,
    MSG_SIM_GET_STATS = 3
} MessageType;

typedef struct {
    MessageType type;
    int x;
    int y;
} Message;

typedef struct {
  int total_steps;
  int max_steps;
  int succ_runs;
  int total_runs;
} StatsMessage;

