#pragma once
#include "types.h"

void load_config(const char* filename, SimulationConfig* config);
void save_config(const char* filename, const SimulationConfig* config);
_Bool validate_config(const SimulationConfig* config);
