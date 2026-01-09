#pragma once
#include "../simulation/simulation.h"

typedef struct {
  Simulation *sim;
  int start_x;
  int start_y;
  int should_exit;
} ServerState;

