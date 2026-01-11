#pragma once

typedef struct {
  int x;
  int y;
} Position;

typedef enum {
  MODE_INTERACTIVE,
  MODE_SUMMARY
} DisplayMode;

typedef enum {
  DISPLAY_AVERAGE_STEPS,
  DISPLAY_PROBABILITY
} DisplayType;

typedef struct {
  Position pos;
  Position start_pos;
  int steps;
  _Bool reached_center;
} WalkerState;

typedef struct {
  double up;
  double down;
  double left;
  double right;
} MoveProbabilities;

typedef struct {
  int width;
  int height;
  int x;
  int y;
  MoveProbabilities probs;
  int max_steps_K;
  int total_replications;
  int current_replication;
  double obstacle_ratio;
  DisplayMode mode;
} SimulationConfig;
