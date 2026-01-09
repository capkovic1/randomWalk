#include "server.h"
#include "server_state.h"
#include "../common/common.h"
#include "../common/ipc.h"
#include "../simulation/simulation.h"

#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* client_thread_func(void* arg) {
    ClientThreadData *data = (ClientThreadData*)arg;
    
    Message msg;
    if (read(data->client_fd, &msg, sizeof(msg)) > 0) {
        // P11: Zamkneme simuláciu, aby k nej iné vlákno v tomto čase nepristupovalo
        pthread_mutex_lock(data->mutex);
        
        handle_message(data->state, data->client_fd, &msg);
        
        pthread_mutex_unlock(data->mutex);
    }

    close(data->client_fd);
    free(data); // Uvoľníme pomocnú štruktúru
    return NULL;
}

void handle_message(ServerState *state, int client_fd, Message *msg) {

    int start_x = msg->x;
    int start_y = msg->y;

    // =========================
    // 1. SPRACUJ PRÍKAZ
    // =========================

    if (msg->type == MSG_SIM_RUN) {

        printf("[SERVER] SIM_RUN\n");
        simulation_run(state->sim, (Position){msg->x, msg->y});
        if(state->sim->stats->total_runs >= state->sim->config.total_replications) {
          state->should_exit = 1;
    }    

    } else if (msg->type == MSG_SIM_STEP) {

        printf("[SERVER] SIM_STEP\n");
        //simulate_interactive(state->sim, mutex);     
        walker_move(state->sim->walker, state->sim->world);
        
        // Overenie či sa dostal na cieľ
        if(state->sim->walker->at_finish) {
            state->should_exit = 1;
        }
        
        // Overenie či prekročil max kroky
        if(state->sim->walker->steps_made >= state->sim->config.max_steps_K) {
            state->should_exit = 1;
        }

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
        out.max_steps = state->sim->config.max_steps_K;
        out.remaining_runs = state->sim->config.total_replications - state->sim->stats->total_runs;
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
            .total_replications = msg->replications,
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
        ack.max_steps = msg->max_steps;
        ack.remaining_runs = msg->replications;  // Všetky behy sú ešte na začiatku

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
    out.max_steps   = state->sim->config.max_steps_K;
    out.width       = state->sim->world->width;
    out.height      = state->sim->world->height;
    out.posX        = state->sim->walker->pos.x;
    out.posY        = state->sim->walker->pos.y;
    out.finished    = state->should_exit;
    out.remaining_runs = state->sim->config.total_replications - state->sim->stats->total_runs;
    
    // Vypočítaj success rate
    if (out.total_runs > 0) {
        out.success_rate = (100.0 * out.succ_runs) / out.total_runs;
    } else {
        out.success_rate = 0.0;
    }

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


void server_run(const char * socket_path) {
    ServerState state = {0};
    state.should_exit = 0;
    pthread_mutex_t sim_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex pre P11
    
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path , sizeof(addr.sun_path) - 1);

    unlink(socket_path);
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10); // P4: Zvýšime frontu pre viac klientov

    // P10: Registruj server v centrálnom registri
    register_server(socket_path, 50, 50);

    printf("[SERVER] Multithreaded server ready on %s\n", socket_path);

    while (!state.should_exit) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        // P4: Pre každého klienta alokujeme dáta a spustíme vlákno
        ClientThreadData *data = malloc(sizeof(ClientThreadData));
        data->client_fd = client_fd;
        data->state = &state;
        data->mutex = &sim_mutex;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread_func, data) == 0) {
            pthread_detach(tid); // Vlákno sa po skončení samo vyčistí
        } else {
            close(client_fd);
            free(data);
        }
    }
   
    close(server_fd);
    unlink(socket_path);
    
    // P10: Deregistruj server z registra
    unregister_server(socket_path);

    if(state.sim)simulation_destroy(state.sim);
    pthread_mutex_destroy(&sim_mutex);
}

