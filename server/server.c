#include "server.h"
#include "server_state.h"
#include "../common/common.h"
#include "../simulation/simulation.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void server_run(void) {
    ServerState state = {0};

    SimulationConfig config = {
        .width = 11,
        .height = 11,
        .max_steps_K = 1000,
        .current_replication = 0,
        .probs = (MoveProbabilities){25,25,25,25}
    };

    state.sim = simulation_create(config);
   Statistics * stats = state.sim->stats;

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("[SERVER] Ready\n");

while (1) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) continue;

    Message msg;
    read(client_fd, &msg, sizeof(msg));

    if (msg.type == MSG_SIM_RUN) {
        printf("[SERVER] SIM_RUN\n");
        simulation_run(state.sim, (Position){msg.x, msg.y});
    }
    else if (msg.type == MSG_SIM_RESET) {
        printf("[SERVER] SIM_RESET\n");
        reset_stats(stats);
    }
    
    
    StatsMessage out = {
        .total_runs = stats->total_runs,
        .succ_runs = stats->succ_runs,
        .total_steps = stats->total_steps
    };

    write(client_fd, &out, sizeof(out));
    close(client_fd);
}
}

