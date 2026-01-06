#include "world.h"
#include <stdlib.h>

World* world_create(int width , int height) {
  World * world = malloc(sizeof(World));

  world->height = height;
  world->width = width;

  world->obstacle = malloc(height * sizeof(_Bool*));

  world->visited = malloc(height * sizeof(_Bool*)); 

  for (int i = 0; i < height; i++) {
    world->obstacle[i] = malloc(width *sizeof(_Bool));
    world->visited[i] = malloc(width * sizeof(_Bool));
  }   

  return world;
}
void world_destroy(World *world) {
  
  for (int i  = 0; i < world->height; i++) {

    free(world->obstacle[i]);
    free(world->visited[i]);
  }
  
  free(world->obstacle);
  free(world->visited);
  free(world);
}
_Bool world_is_valid_position(World *world, Position pos) {
  if (pos.x < 0|| pos.y < 0 || pos.x >= world->width || pos.y >= world->height) {
    return 0;
  }
  return 1;
}
_Bool world_add_obstacle(World *world, Position pos) {
  if (world_is_valid_position(world, pos) && world->obstacle[pos.x][pos.y] == 0
  ) {
    world->obstacle[pos.x][pos.y] = 1;
    return 1;
  }
  return 0;
 
}
_Bool world_remove_obstacle(World *world, Position pos) {
  if (world_is_valid_position(world,pos) && world->obstacle[pos.x][pos.y]==1) {
    world->obstacle[pos.x][pos.y] = 0;
    return 1;
  }
  return 0;
}

_Bool world_is_accessible(World *world, Position to) {
  

  if (world_is_valid_position(world, to) && world->obstacle[to.x][to.y]==0) {
    return 1;
  } else {
    return 0;
  }

}

World* world_generate_random(int width , int height , double obstacle_ratio){
  World *world = world_create(width, height);
  Position pos;

  if (obstacle_ratio >= 1) {
    obstacle_ratio /=100;
  }


  int num_of_obstacle = (width * height) * obstacle_ratio;

  int index = 0;
  while ( index < num_of_obstacle) {
    pos.x = rand() %width;
    pos.y = rand() %height;

    if(world_add_obstacle(world, pos)){
      index++;
    }
  }

  return world;

}

void reset_visited(World * world){
  for (int i = 0; i < world->height; i++) {
    for (int j = 0; j < world->width; j++) {
      world->visited[i][j] = 0;
    }
  }
}
