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
  if (pos.x < 0|| pos.y < 0 || pos.x > 50 || pos.y > 50) {
    return 0;
  }
  return 1;
}
_Bool world_add_obstacle(World *world, Position pos) {
  if (world_is_valid_position(world, pos) && world->obstacle[pos.y][pos.x] == 0) {
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
  

  if (world_is_valid_position(world, to) && world->obstacle[to.y][to.x]==0 ) {
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
    // Ak začíname v cieli, cesta existuje hneď
    if (start.x == 0 && start.y == 0) return 1;

    // 1. Alokácia dočasnej mapy navštívených políčok pre BFS
    _Bool **visited_tmp = malloc(world->height * sizeof(_Bool*));
    for (int i = 0; i < world->height; i++) {
        visited_tmp[i] = calloc(world->width, sizeof(_Bool));
    }

    // 2. Príprava fronty (queue) pre BFS
    Position *queue = malloc(world->width * world->height * sizeof(Position));
    int head = 0, tail = 0;

    // Vložíme štartovaciu pozíciu
    queue[tail++] = start;
    visited_tmp[start.y][start.x] = 1;

    // Smery pohybu: hore, dole, vľavo, vpravo
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    _Bool found = 0;

    // 3. Hlavný cyklus BFS
    while (head < tail) {
        Position curr = queue[head++];

        // Skontrolujeme, či sme v cieli [0,0]
        if (curr.x == 0 && curr.y == 0) {
            found = 1;
            break;
        }

        for (int i = 0; i < 4; i++) {
            Position next = {curr.x + dx[i], curr.y + dy[i]};

            // PAC-MAN LOGIKA: Pretečenie cez okraje (ako v tvojom walker_move)
            if (next.x >= world->width) next.x = 0;
            else if (next.x < 0) next.x = world->width - 1;

            if (next.y >= world->height) next.y = 0;
            else if (next.y < 0) next.y = world->height - 1;

            // Kontrola, či na novej pozícii nie je prekážka a či sme tam už neboli
            // Používame [y][x] kvôli tvojej alokácii v world_create
            if (!world->obstacle[next.y][next.x] && !visited_tmp[next.y][next.x]) {
                visited_tmp[next.y][next.x] = 1;
                queue[tail++] = next;
            }
        }
    }

    // 4. Uvoľnenie pamäte
    for (int i = 0; i < world->height; i++) {
        free(visited_tmp[i]);
    }
    free(visited_tmp);
    free(queue);

    return found;
}


