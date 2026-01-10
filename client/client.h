#pragma once

#include "../common/common.h"

void client_run(void);
void* receiver_thread_func(void* arg);
StatsMessage send_command(const char* socket_path, MessageType type, int x, int y); 
