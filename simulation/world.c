#include "world.h"
#include <stdlib.h>

World* world_create(int width , int height) {
  World * world = malloc(sizeof(World));

  world->height = height;
  world->width = width;

  world->obstacle = malloc(height * sizeof(_Bool*));

  world->visited = malloc(height * sizeof(_Bool*)); 

  for (int i = 0; i < height; i++) {
    world->obstacle[i] = calloc(width, sizeof(_Bool));
    world->visited[i] = calloc(width , sizeof(_Bool));
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
  if (world_is_valid_position(world, pos) && world->obstacle[pos.y][pos.x] == 0
  ) {
    world->obstacle[pos.y][pos.x] = 1;
    return 1;
  }
  return 0;
 
}
_Bool world_remove_obstacle(World *world, Position pos) {
  if (world_is_valid_position(world,pos) && world->obstacle[pos.y][pos.x]==1) {
    world->obstacle[pos.y][pos.x] = 0;
    return 1;
  }
  return 0;
}

_Bool world_is_accessible(World *world, Position to) {
  

  if (world_is_valid_position(world, to) && world->obstacle[to.y][to.x]==0) {
    return 1;
  } else {
    return 0;
  }

}

World* world_generate_random(int width , int height , double obstacle_ratio , Position startPos){
  World *world = world_create(width, height);


  if (obstacle_ratio >= 1) {
    obstacle_ratio /=100;
  }
  int num_of_obstacle = (width * height) * obstacle_ratio;
  Position pos;

  int index = 0;
  while ( index < num_of_obstacle) {
    
    pos.x = rand() %width;
    pos.y = rand() %height;

    if(world_add_obstacle(world, pos) || (startPos.x == pos.x && startPos.y == pos.y)){
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
void reset_obstacles(World * world) {
  for (int i = 0; i < world->height; i++) {
    for (int j = 0; j < world->width; j++) {
      world->obstacle[i][j] = 0;
    }
  }
}
_Bool world_has_path(World *world, Position start) {
    if (start.x == 0 && start.y == 0) return 1;

    // Dočasná mapa pre BFS, aby sme si nepoškodili world->visited
    _Bool **visited = malloc(world->height * sizeof(_Bool*));
    for(int i = 0; i < world->height; i++) visited[i] = calloc(world->width, sizeof(_Bool));

    Position queue[world->width * world->height];
    int head = 0, tail = 0;

    queue[tail++] = start;
    visited[start.y][start.x] = 1;

    const int dx[] = {0, 0, 1, -1};
    const int dy[] = {1, -1, 0, 0};

    while(head < tail) {
        Position curr = queue[head++];
        if(curr.x == 0 && curr.y == 0) {
            // Cesta nájdená! Vyčisti pamäť a vráť true
            for(int i=0; i<world->height; i++) free(visited[i]);
            free(visited);
            return 1;
        }

        for(int i = 0; i < 4; i++) {
            Position next = {curr.x + dx[i], curr.y + dy[i]};
            if(world_is_valid_position(world, next) && 
               !world->obstacle[next.y][next.x] && !visited[next.y][next.x]) {
                visited[next.y][next.x] = 1;
                queue[tail++] = next;
            }
        }
    }

    for(int i=0; i<world->height; i++) free(visited[i]);
    free(visited);
    return 0; // Cesta neexistuje
}


