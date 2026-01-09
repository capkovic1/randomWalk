#pragma once

#include "server_state.h"

#include "../common/common.h"
typedef struct {
    int client_fd;
    ServerState *state;
    pthread_mutex_t *mutex;
} ClientThreadData;

void server_run(const char * socket_path);
void handle_message(ServerState * state , int client_fd , Message * msg, pthread_mutex_t *mutex);

