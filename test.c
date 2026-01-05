#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include "simulation/simulation.h"
#include "simulation/world.h"

int main() {

  srand(time(NULL));

  MoveProbabilities probs;
  probs.down = 25;
  probs.left = 25;
  probs.right = 25;
  probs.up = 25;

  SimulationConfig config;

  config.width = 10;
  config.height = 10;
  config.current_replication = 0;
  config.total_replications = 50;
  config.max_steps_K = 500;
  config.probs =  probs;
  config.mode = MODE_SUMMARY;
  

  Simulation *sim = simulation_create(config);

  simulation_run(sim, (Position){5,5});
  
  print_stats(sim->stats); 

  reset_stats(sim->stats);

  simulation_run_n_times(sim, (Position){5,5},100);
  print_stats(sim->stats);

  return 0;
} 
