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

    // 1. NAJPRV VYKONAJ AKCIU PODĽA TYPU SPRÁVY
    if (msg.type == MSG_SIM_RUN) {
      
      printf("[SERVER] SIM_RUN\n");
      simulation_run(state.sim, (Position){msg.x, msg.y});
    
    } else if (msg.type == MSG_SIM_STEP) {
      
      printf("[SERVER] SIM_STEP\n");
      walker_move(state.sim->walker, state.sim->world);
    
    } else if (msg.type == MSG_SIM_RESET) {

      printf("[SERVER] SIM_RESET\n");
      reset_stats(stats);
      walker_reset(state.sim->walker, (Position){msg.x , msg.y});
      reset_visited(state.sim->world); 

    } else if (msg.type == MSG_SIM_INIT) {
        printf("[SERVER] SIM_INIT\n");
        state.sim->walker->pos.x = msg.x;
        state.sim->walker->pos.y = msg.y;
    }

    // 2. AŽ TERAZ NAPLŇ ODPOVEĎ ČERSTVÝMI DÁTAMI
    StatsMessage out;
    memset(&out, 0, sizeof(out));

    // Základné údaje (musia byť po simulácii, aby boli aktuálne)
    out.curr_steps = state.sim->walker->steps_made;
    out.total_runs = state.sim->stats->total_runs;
    out.succ_runs =  state.sim->stats->succ_runs;
    out.total_steps =  state.sim->stats->total_steps;
    out.width = state.sim->world->width;
    out.height = state.sim->world->height;
    out.posX = state.sim->walker->pos.x;
    out.posY = state.sim->walker->pos.y;

    // Kopírovanie polí s ochranou proti pretečeniu (max 50x50)
    for (int y = 0; y < out.height && y < 50; y++) {
        for (int x = 0; x < out.width && x < 50; x++) {
            out.visited[y][x] = state.sim->world->visited[y][x];
            out.obstacle[y][x] = state.sim->world->obstacle[y][x];
        }
    }

    // 3. ODPOŠLI A ZAVRI
    write(client_fd, &out, sizeof(out));
    close(client_fd);
  }
}

