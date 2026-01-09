#include "server.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Pouzitie: %s <cesta_k_socketu>\n", argv[0]);
        return 1;
    }

    char *socket_path = argv[1];
    server_run(socket_path);

    return 0;
}


