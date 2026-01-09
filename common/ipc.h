#pragma once

#include <sys/socket.h>
#include <sys/un.h>

typedef struct {
    char socket_path[256];
    int width;
    int height;
} ServerInfo;

// Registrácia servera v centrálnom registri
int register_server(const char *socket_path, int width, int height);

// Deregistrácia servera
int unregister_server(const char *socket_path);

// Skontroluj, či je server dostupný
int server_is_alive(const char *socket_path);

// Načítaj zoznam dostupných serverov
ServerInfo* list_available_servers(int *count);

// Vyčisti mŕtve servery z registra
void cleanup_dead_servers(void);
