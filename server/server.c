#include "server.h"
#include "server_state.h"
#include "../common/common.h"
#include "../simulation/simulation.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


void handle_message(ServerState *state, int client_fd, Message *msg) {

    int start_x = msg->x;
    int start_y = msg->y;

    // =========================
    // 1. SPRACUJ PRÍKAZ
    // =========================

    if (msg->type == MSG_SIM_RUN) {

        printf("[SERVER] SIM_RUN\n");
        simulation_run(state->sim, (Position){msg->x, msg->y});

    } else if (msg->type == MSG_SIM_STEP) {

        printf("[SERVER] SIM_STEP\n");
        walker_move(state->sim->walker, state->sim->world);

    } else if (msg->type == MSG_SIM_RESET) {

        if (state->sim != NULL) {
            printf("[SERVER] SIM_RESET to (%d , %d)\n",
                   state->start_x, state->start_y);

            reset_stats(state->sim->stats);
            walker_reset(
                state->sim->walker,
                (Position){state->start_x, state->start_y}
            );
            reset_visited(state->sim->world);
            reset_obstacles(state->sim->world);
        }

    } else if (msg->type == MSG_SIM_INIT) {

        printf("[SERVER] SIM_INIT\n");

        state->sim->walker->pos.x = msg->x;
        state->sim->walker->pos.y = msg->y;

        state->sim->world->visited[msg->y][msg->x] = 1;

        StatsMessage out = {0};
        out.width = state->sim->world->width;
        out.height = state->sim->world->height;
        out.posX = state->sim->walker->pos.x;
        out.posY = state->sim->walker->pos.y;

        for (int y = 0; y < out.height && y < 50; y++) {
            for (int x = 0; x < out.width && x < 50; x++) {
                out.visited[y][x] = state->sim->world->visited[y][x];
                out.obstacle[y][x] = state->sim->world->obstacle[y][x];
            }
        }

        write(client_fd, &out, sizeof(out));
        return;   // 🔴 POZOR – neodosiela sa druhá odpoveď
    }

    else if (msg->type == MSG_SIM_CONFIG) {

        printf("[SERVER] Reconfiguring simulation...\n");

        if (state->sim != NULL) {
            simulation_destroy(state->sim);
        }

        SimulationConfig new_config = {
            .width = msg->width,
            .height = msg->height,
            .max_steps_K = msg->max_steps,
            .probs = (MoveProbabilities){
                msg->probs[0],
                msg->probs[1],
                msg->probs[2],
                msg->probs[3]
            }
        };

        state->sim = simulation_create(new_config);

        world_destroy(state->sim->world);
        state->sim->world = create_guaranteed_world(
            msg->width,
            msg->height,
            0.1,
            (Position){msg->x, msg->y}
        );

        state->start_x = msg->x;
        state->start_y = msg->y;

        state->sim->walker->pos.x = msg->x;
        state->sim->walker->pos.y = msg->y;

        state->sim->world->visited[start_y][start_x] = 1;

        StatsMessage ack = {0};
        ack.width = msg->width;
        ack.height = msg->height;
        ack.posX = msg->x;
        ack.posY = msg->y;

        for (int y = 0; y < ack.height && y < 50; y++) {
            for (int x = 0; x < ack.width && x < 50; x++) {
                ack.obstacle[y][x] = state->sim->world->obstacle[y][x];
                ack.visited[y][x] = state->sim->world->visited[y][x];
            }
        }

        write(client_fd, &ack, sizeof(ack));
        return;   // 🔴 dôležité
    }

    // =========================
    // 2. VYTVOR ODPOVEĎ
    // =========================

    StatsMessage out;
    memset(&out, 0, sizeof(out));

    out.curr_steps  = state->sim->walker->steps_made;
    out.total_runs  = state->sim->stats->total_runs;
    out.succ_runs   = state->sim->stats->succ_runs;
    out.total_steps = state->sim->stats->total_steps;
    out.width       = state->sim->world->width;
    out.height      = state->sim->world->height;
    out.posX        = state->sim->walker->pos.x;
    out.posY        = state->sim->walker->pos.y;

    for (int y = 0; y < out.height && y < 50; y++) {
        for (int x = 0; x < out.width && x < 50; x++) {
            out.visited[y][x]  = state->sim->world->visited[y][x];
            out.obstacle[y][x] = state->sim->world->obstacle[y][x];
        }
    }

    // =========================
    // 3. ODPOŠLI
    // =========================

    write(client_fd, &out, sizeof(out));
}


void server_run(void) {
  
  ServerState state = {0};
  state.sim = NULL;
  Statistics * stats = NULL;

  int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, SOCKET_PATH);

  unlink(SOCKET_PATH);
  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr))) {
    perror("bind");
    return;
  }
  listen(server_fd, 5);

  printf("[SERVER] Ready\n");

 while (1) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) continue;

    Message msg;
    int r = read(client_fd, &msg, sizeof(msg));
    if (r <= 0) {
        close(client_fd);
        continue;
    }

    // 🔹 TU VOLÁME NOVÚ FUNKCIU handle_message
    handle_message(&state, client_fd, &msg);

    close(client_fd);  // server zatvorí socket po spracovaní správy

  }
}

