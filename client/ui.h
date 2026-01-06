#include "../common/common.h"

UIState draw_mode_menu(int *mode);
void draw_setup(int *x, int *y, int *K, int *runs, int mode);
void draw_world(int height , int width , int posX , int posY , _Bool** obstacle , _Bool** visited); 
void draw_summary(double grid[11][11], int w, int h);
void draw_stats(StatsMessage *s , int offset_y , UIState state); 
