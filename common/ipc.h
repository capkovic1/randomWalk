#pragma once

#include <sys/socket.h>
#include <sys/un.h>

typedef struct {
    char socket_path[256];
    int width;
    int height;
} ServerInfo;

int register_server(const char *socket_path, int width, int height);
int unregister_server(const char *socket_path);
int server_is_alive(const char *socket_path);
ServerInfo* list_available_servers(int *count);
void cleanup_dead_servers(void);
