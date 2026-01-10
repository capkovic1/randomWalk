#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Súbor s registrom aktívnych serverov
#define REGISTRY_FILE "/tmp/drunk_servers_registry.txt"

int register_server(const char *socket_path, int width, int height) {
    FILE *f = fopen(REGISTRY_FILE, "a");
    if (!f) {
        return -1;
    }
    
    fprintf(f, "%s|%d|%d\n", socket_path, width, height);
    fclose(f);
    return 0;
}

int unregister_server(const char *socket_path) {
    FILE *f = fopen(REGISTRY_FILE, "r");
    FILE *tmp = fopen(REGISTRY_FILE ".tmp", "w");
    
    if (!f || !tmp) {
        if (f) fclose(f);
        if (tmp) fclose(tmp);
        return -1;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, socket_path, strlen(socket_path)) != 0) {
            fputs(line, tmp);
        }
    }
    
    fclose(f);
    fclose(tmp);
    rename(REGISTRY_FILE ".tmp", REGISTRY_FILE);
    
    return 0;
}

// Skontroluj, či socket existuje a je dostupný
int server_is_alive(const char *socket_path) {
    int test_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (test_fd < 0) return 0;
    
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    int result = (connect(test_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    close(test_fd);
    
    return result;
}

// Načítaj zoznam dostupných serverov
ServerInfo* list_available_servers(int *count) {
    FILE *f = fopen(REGISTRY_FILE, "r");
    if (!f) {
        *count = 0;
        return NULL;
    }
    
    // Prvý prechod: spočítaj platné servery
    int temp_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Skontroluj, či je socket ešte aktívny
        char socket_path[256] = {0};
        int w, h;
        if (sscanf(line, "%255[^|]|%d|%d", socket_path, &w, &h) == 3) {
            if (server_is_alive(socket_path)) {
                temp_count++;
            }
        }
    }
    
    rewind(f);
    
    // Druhý prechod: načítaj platné servery
    ServerInfo *servers = malloc(temp_count * sizeof(ServerInfo));
    if (!servers) {
        fclose(f);
        *count = 0;
        return NULL;
    }
    int idx = 0;
    
    while (fgets(line, sizeof(line), f) && idx < temp_count) {
        char socket_path[256] = {0};
        int w, h;
        if (sscanf(line, "%255[^|]|%d|%d", socket_path, &w, &h) == 3) {
            if (server_is_alive(socket_path)) {
                strncpy(servers[idx].socket_path, socket_path, 255);
                servers[idx].width = w;
                servers[idx].height = h;
                idx++;
            }
        }
    }
    
    fclose(f);
    *count = idx;
    return servers;
}

void cleanup_dead_servers(void) {
    FILE *f = fopen(REGISTRY_FILE, "r");
    FILE *tmp = fopen(REGISTRY_FILE ".tmp", "w");
    
    if (!f || !tmp) {
        if (f) fclose(f);
        if (tmp) fclose(tmp);
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char socket_path[256] = {0};
        int w, h;
        if (sscanf(line, "%255[^|]|%d|%d", socket_path, &w, &h) == 3) {
            if (server_is_alive(socket_path)) {
                fputs(line, tmp);
            }
        }
    }
    
    fclose(f);
    fclose(tmp);
    rename(REGISTRY_FILE ".tmp", REGISTRY_FILE);
}
