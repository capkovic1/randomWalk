#ifndef MENU_HANDLER_H
#define MENU_HANDLER_H

#include "../common/common.h"

UIState handle_create_new_server(ClientContext *ctx,char *room_code,int *mode);
UIState handle_connect_to_existing(ClientContext *ctx);
int wait_for_server(const char *socket_path, int max_retries);
int send_config_to_server(ClientContext *ctx,int x, int y,int width, int height,int K, int runs,int *probs,const char *out_filename,UIState *next_state);
void show_error_dialog(const char *message);

#endif
