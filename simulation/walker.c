#include "walker.h"
#include "world.h"
#include <stdlib.h>

Walker* walker_create(Position start ,MoveProbabilities probs) {
  Walker *walker = malloc(sizeof(Walker));
  if (!walker) return NULL;
  
  walker->pos = start;
  walker->start_pos = start;
  walker->probs = probs;
  walker->at_finish = 0;
  walker->steps_made = 0;
  walker->num_of_sim = 0;
  walker->succ_sim = 0;
  return walker;
}

void walker_destroy(Walker *walker) {
  free(walker);
}

_Bool walker_move(Walker *walker, World *world) {
  MoveProbabilities probs = walker->probs;
  

  if(walker->at_finish) {
    return 1;
  }
 
  int r = rand() % 100;
  Position newPosition = walker->pos;
  
  if(r<probs.down) {
    newPosition.y +=1;
  } else if (r < probs.down + probs.up) {
    newPosition.y -=1;
  } else if (r < probs.left + probs.down + probs.up) {
    newPosition.x -= 1;
  } else {
    newPosition.x += 1;
  }
    
  if(newPosition.x >= world->width) {
    newPosition.x %= world->width;
  }
  if (newPosition.y >= world->height) {
    newPosition.y %= world->height;
  }
  
  if (newPosition.x < 0) {
    newPosition.x = world->width - 1;
  }
  if (newPosition.y < 0) {
    newPosition.y = world->height - 1;
  }


  if(world_is_valid_position(world, newPosition) && !world->obstacle[newPosition.y][newPosition.x]) {
    if ((walker->pos.x > 0 && walker->pos.x < 50) || (walker->pos.y > 0 && walker->pos.y < 50)){
      world->visited[walker->pos.y][walker->pos.x] = 1;
    }
    walker->pos = newPosition;
    walker->steps_made++;
    if(walker->pos.x == 0 && walker->pos.y == 0) {
      walker->at_finish = 1;
    }
    return 1;
  }
  return 0;
}

void walker_reset(Walker *walker, Position start) {
  walker->at_finish = 0;
  walker->pos = start;
  walker->steps_made = 0;
  walker->start_pos = start;
}

_Bool walker_has_reached_center(const Walker *walker) {
  return walker->at_finish;
}

Position walker_get_position(const Walker *walker) {
  return walker->pos;
}
int walker_get_steps(const Walker *walker) {
  return walker->steps_made;
}
Trajectory * trajectory_create(int max_length) {
  Trajectory * trajectory = malloc(sizeof(Trajectory));
  if (!trajectory) return NULL;
  
  trajectory->pos = malloc(max_length * sizeof(Position));
  if (!trajectory->pos) {
    free(trajectory);
    return NULL;
  }

  trajectory->capacity = max_length;
  trajectory->steps_made = 0;
  trajectory->length = 0;
  trajectory->finished = 0;

  return trajectory;
}

void trajectory_destroy(Trajectory *traj) {
  if (!traj) return;
  free(traj->pos);
  free(traj);
}

void trajectory_add_pos(Trajectory *traj, Position pos) {
  traj->pos[traj->length] = pos;
  traj->length++;
}

Trajectory * walker_simulate_to_center(Walker * walker , World * world , int max_steps) {
  Trajectory * trajectory = trajectory_create(max_steps);
  trajectory_add_pos(trajectory, walker->start_pos);
  if (walker->pos.x ==0 && walker->pos.y == 0) {
    trajectory->finished = 1;
    walker->succ_sim++;
    walker->at_finish =1;
    return trajectory;
  }
  while (!walker->at_finish && walker->steps_made < max_steps) {
      if(walker_move(walker , world)){
      trajectory_add_pos(trajectory, walker->pos);
    }
  }
  if (walker->at_finish) {
    trajectory->finished = 1;
    walker->succ_sim++;
  }

  trajectory->steps_made = walker->steps_made;
  walker->num_of_sim++;

  return trajectory;

} 
