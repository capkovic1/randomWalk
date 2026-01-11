#ifndef WALKER_H
#define WALKER_H

#include "world.h"


typedef struct {
  Position pos;
  Position start_pos;
  MoveProbabilities probs;
  int steps_made;
  _Bool at_finish;
  int num_of_sim;
  int succ_sim;
} Walker ;

typedef struct {
  Position* pos;
  int capacity;
  int length;
  _Bool finished;
  int steps_made;
}Trajectory ;


  
Trajectory* trajectory_create(int max_length);
void trajectory_destroy(Trajectory* traj);
void trajectory_add_pos(Trajectory* traj , Position pos );


Walker* walker_create(Position start, MoveProbabilities probs);
void walker_destroy(Walker* walker);
_Bool walker_move(Walker* walker, World* world);
void walker_reset(Walker* walker, Position start);
_Bool walker_has_reached_center(const Walker* walker);
Position walker_get_position(const Walker* walker);
int walker_get_steps(const Walker* walker);
Trajectory* walker_simulate_to_center(Walker* walker, World* world, int max_steps);
#endif 
