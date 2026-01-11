#include "simulation.h"
#include "world.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

Statistics * stat_create() {
  Statistics * stats = malloc(sizeof(Statistics));
  if (stats) {
    stats->total_steps = 0;
    stats->max_steps = 0;
    stats->succ_runs = 0;
    stats->total_runs = 0;
  }
  return stats;
}
void stat_destroy(Statistics *stat) {
  free(stat);
}

Simulation * simulation_create(SimulationConfig config) {
  if(config.width <= 0 || config.height <= 0) {
    return NULL;
  }
  
  Simulation * sim = malloc(sizeof(Simulation));
  if (!sim) return NULL;
  
  sim->world = world_create(config.width, config.height);
  if (!sim->world) {
    free(sim);
    return NULL;
  }
  
  Position pos =  {config.x , config.y};
  sim->walker = walker_create(pos, config.probs);
  if (!sim->walker) {
    world_destroy(sim->world);
    free(sim);
    return NULL;
  }

  sim->config = config;
  sim->stats = stat_create();
  if (!sim->stats) {
    walker_destroy(sim->walker);
    world_destroy(sim->world);
    free(sim);
    return NULL;
  }
  sim->filename = NULL;

  return sim;
}

void simulation_destroy(Simulation *sim) {
  walker_destroy(sim->walker);
  world_destroy(sim->world);
  stat_destroy(sim->stats);
  if (sim->filename) free(sim->filename);
  free(sim);
}
_Bool simulation_run(Simulation *sim , Position pos) {
  SimulationConfig config = sim->config;

      if (!world_is_accessible(sim->world, pos)) {
        return 0;
      }
      walker_reset(sim->walker, pos);
      
      Trajectory * traj = walker_simulate_to_center(sim->walker, sim->world, config.max_steps_K);

      if (sim->filename && strlen(sim->filename) > 0) {
        FILE *f = fopen(sim->filename, "a");
        if (f) {
          int run_no = sim->config.current_replication + 1;
          fprintf(f, "Run %d:\n", run_no);
          fprintf(f, "steps=%d success=%d\n", traj->steps_made, walker_has_reached_center(sim->walker));
          for (int i = 0; i < traj->length; i++) {
            fprintf(f, "%d %d\n", traj->pos[i].x, traj->pos[i].y);
          }
          fprintf(f, "---\n");
          fclose(f);
        }
      }

      sim->stats->total_steps += traj->steps_made;
      sim->stats->total_runs ++;
      sim->stats->max_steps += config.max_steps_K;
      sim->config.current_replication++;

      if(walker_has_reached_center(sim->walker)) {
        sim->stats->succ_runs++;
      }
      trajectory_destroy(traj);

  return 1;
  
}
_Bool simulate_interactive(Simulation *sim, pthread_mutex_t *mutex) {
    while (1) {
        pthread_mutex_lock(mutex);
        
        if (sim->walker->at_finish) {
            pthread_mutex_unlock(mutex);
            break;
        }

        walker_move(sim->walker, sim->world);
        pthread_mutex_unlock(mutex);
        usleep(100000); 
    }
    return 1;
}

_Bool simulation_run_n_times(Simulation * sim, Position pos, int times) {
  if (!world_is_accessible(sim->world, pos)) {
    return 0;
  }

  for (int  i = 0; i < times; i++) {
    simulation_run(sim, pos);
  }

  return 1;
}

_Bool simulation_save_results(Simulation* sim, const char* filename) {
  if (!filename || strlen(filename) == 0) return 0;
  FILE *f = fopen(filename, "a");
  if (!f) return 0;
  int total = sim->stats->total_runs;
  int succ = sim->stats->succ_runs;
  int steps = sim->stats->total_steps;
  double success_rate = total > 0 ? ((double)succ * 100.0) / (double)total : 0.0;
  fprintf(f, "SUMMARY:\n");
  fprintf(f, "total_runs=%d succ_runs=%d total_steps=%d success_rate=%.2f\n", total, succ, steps, success_rate);
  fprintf(f, "EOF\n\n");
  fclose(f);
  return 1;
}

void reset_stats(Statistics *stats) {
  stats->succ_runs = 0;
  stats->total_runs = 0;
  stats->total_steps = 0;
}

World* create_guaranteed_world(int w, int h, double ratio, Position start) {
    World* world = NULL;
    int max_attempts = 100;
    int attempts = 0;
    
    do {
        if (world) world_destroy(world);
        world = world_generate_random(w, h, ratio, start);
        if (!world) return NULL;
        
        _Bool has_path = world_has_path(world, start);
        if (has_path) {
            return world;
        }
        attempts++;
    } while (attempts < max_attempts);
    
    // All attempts failed
    if (world) {
        world_destroy(world);
    }
    return NULL;
}

