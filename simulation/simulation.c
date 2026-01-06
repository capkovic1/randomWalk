#include "simulation.h"
#include "world.h"
#include <stdio.h>
#include <stdlib.h>

Statistics * stat_create() {
  Statistics * stats = malloc(sizeof(Statistics));
  return stats;
}
void stat_destroy(Statistics *stat) {
  free(stat);
}

Simulation * simulation_create(SimulationConfig config) {
 
  if(config.width <= 0 || config.height <= 0) {
    printf("ERROR: Nespravne rozmery\n");
    return NULL;
  }
  
  Simulation * sim = malloc(sizeof(Simulation));

  sim->world = world_create(config.width, config.height);
  
  Position pos;
  pos.x = 5;
  pos.y = 5;

  sim->walker = walker_create(pos, config.probs);

  sim->config = config;
  sim->config.current_replication = 0;
  sim->stats = stat_create();

  return sim;
}

void simulation_destroy(Simulation *sim) {
  walker_destroy(sim->walker);
  world_destroy(sim->world);
  stat_destroy(sim->stats);
  free(sim);
}
_Bool simulation_run(Simulation *sim , Position pos) {
  SimulationConfig config = sim->config;

      if (!world_is_accessible(sim->world, pos)) {
        return 0;
      }
      walker_reset(sim->walker, pos);
      
      Trajectory * traj = walker_simulate_to_center(sim->walker, sim->world, config.max_steps_K);

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

_Bool simulation_run_n_times(Simulation * sim, Position pos, int times) {
  if (!world_is_accessible(sim->world, pos)) {
    return 0;
  }

  for (int  i = 0; i < times; i++) {
    simulation_run(sim, pos);
  }

  return 1;
}

void print_stats(Statistics *stats) {
  printf("VYSLEDKY SIMULACII SU\n");
  printf("Pocet uspesnych runov je %d z celkovych %d \n",stats->succ_runs,stats->total_runs);
  printf("Urobilo sa celkovo %d krokov\n",stats->total_steps);
  
}

void reset_stats(Statistics *stats) {
  stats->succ_runs = 0;
  stats->total_runs = 0;
  stats->total_steps = 0;
}
