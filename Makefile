CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Icommon -Isimulation -Iserver -Iclient

SERVER_SRC = \
	server/main.c \
	server/server.c \
	simulation/simulation.c \
	simulation/walker.c \
	simulation/world.c

CLIENT_SRC = \
	client/main.c \
	client/client.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

SERVER_BIN = server_app
CLIENT_BIN = client_app

.PHONY: all clean

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ -lncurses

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_BIN) $(CLIENT_BIN)

