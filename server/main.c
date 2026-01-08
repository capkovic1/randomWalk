#include "server.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Pouzitie: %s <cesta_k_socketu>\n", argv[0]);
        return 1;
    }

    // Cesta pride z klienta (napr. /tmp/drunk_mojeid.sock)
    char *socket_path = argv[1];

    printf("[SERVER] Startujem na sockete: %s\n", socket_path);
    server_run(socket_path);

    return 0;
}


