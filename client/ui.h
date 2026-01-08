#include "../common/common.h"

int draw_connection_menu(char* room_code);
UIState draw_mode_menu(int *mode);
UIState draw_setup(int *x, int *y, int *K, int *runs, int *width, int *height, int probs[4], int mode);
void draw_world(int height, int width, int posX, int posY, _Bool obstacle[50][50], _Bool visited[50][50]); 
void draw_summary(double grid[11][11], int w, int h);
void draw_stats(StatsMessage *s , int offset_y , UIState state); 
