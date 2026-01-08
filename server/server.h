#pragma once

#include "server_state.h"
#include "../common/common.h"

void server_run(void);
void handle_message(ServerState * state , int client_fd , Message * msg);

