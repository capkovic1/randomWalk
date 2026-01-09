// simulation.h
#ifndef SIMULATION_H
#define SIMULATION_H

#include "walker.h"
#include "world.h"
#include <pthread.h>

typedef struct {
  int total_steps;
  int max_steps;
  int succ_runs;
  int total_runs;
} Statistics;

typedef struct {
  World * world;
  Walker * walker;
  Statistics* stats;
  SimulationConfig config;

  char* filename;

}Simulation;

Statistics * stat_create();
void stat_destroy(Statistics * stat);

Simulation* simulation_create(SimulationConfig config);
void simulation_destroy(Simulation* sim);
_Bool simulation_run(Simulation* sim , Position pos);
_Bool simulation_run_n_times(Simulation * sim , Position pos , int times);

void reset_stats(Statistics * stats);

Statistics* simulation_get_statistics(Simulation* sim);
_Bool simulation_save_results(Simulation* sim, const char* filename);
World* create_guaranteed_world(int w, int h, double ratio, Position start);
_Bool simulate_interactive(Simulation *sim,  pthread_mutex_t *mutex); 
#endif // SIMULATION_H
