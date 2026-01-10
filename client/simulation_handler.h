#ifndef SIMULATION_HANDLER_H
#define SIMULATION_HANDLER_H

#include "../common/common.h"

// Forward declaration of send_command from client.c
extern StatsMessage send_command(const char* socket_path, MessageType type, int x, int y);

/**
 * Handle interaktívny mód - krok za krokom
 */
void handle_interactive_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K, int runs
);

/**
 * Handle sumárny mód - automatické spúšťanie
 */
void handle_summary_mode(
    ClientContext *ctx,
    StatsMessage *current_stats,
    int x, int y, int K, int runs
);

/**
 * Čítanie vstupov z thread-safe queue
 */
int read_input_from_queue(ClientContext *ctx);

#endif
