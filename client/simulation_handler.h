#ifndef SIMULATION_HANDLER_H
#define SIMULATION_HANDLER_H

#include "../common/common.h"

void handle_interactive_mode(ClientContext *ctx,StatsMessage *current_stats,int x, int y, int K, int runs);
void handle_summary_mode(ClientContext *ctx,StatsMessage *current_stats,int x, int y, int K, int runs);
int read_input_from_queue(ClientContext *ctx);

#endif
