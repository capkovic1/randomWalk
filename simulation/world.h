#ifndef WORLD_H
#define WORLD_H

#include "../common/types.h"  

typedef struct {
  int width;
  int height;
  _Bool** obstacle;
  _Bool** visited;
} World;

// Deklarácie metód
World* world_create(int width, int height);
void world_destroy(World* world);
_Bool world_is_valid_position(World* world, Position pos);
Position world_wrap_position(const World* world, Position pos);
Position* world_get_neighbors(const World* world, Position pos, int* count);
_Bool world_add_obstacle(World* world, Position pos);
_Bool world_remove_obstacle(World* world, Position pos);
_Bool world_is_accessible(World* world, Position to);
_Bool world_load_from_file(World* world, const char* filename);
_Bool world_save_to_file(const World* world, const char* filename);
World* world_generate_random(int width, int height, double obstacle_ratio);
void reset_visited(World * world);
void reset_obstacles(World * world);

#endif 
