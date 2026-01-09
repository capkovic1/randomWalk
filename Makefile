# Kompilátor a flagy
CC = gcc
CFLAGS = -Wall -Wextra -g -I./common -I./client -I./server -I./simulation
LDFLAGS = -lncurses

# Adresáre
CLIENT_DIR = client
SERVER_DIR = server
SIMULATION_DIR = simulation
COMMON_DIR = common

# Všetky zdrojové súbory
CLIENT_SRCS = $(CLIENT_DIR)/main.c $(CLIENT_DIR)/client.c $(CLIENT_DIR)/ui.c
SERVER_SRCS = $(SERVER_DIR)/main.c $(SERVER_DIR)/server.c
SIMULATION_SRCS = $(SIMULATION_DIR)/simulation.c $(SIMULATION_DIR)/walker.c $(SIMULATION_DIR)/world.c

# Objektové súbory
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
SIMULATION_OBJS = $(SIMULATION_SRCS:.c=.o)
COMMON_OBJS = $(COMMON_DIR)/ipc.o

# Hlavičkové súbory
CLIENT_HDRS = $(CLIENT_DIR)/client.h $(CLIENT_DIR)/ui.h
SERVER_HDRS = $(SERVER_DIR)/server.h $(SERVER_DIR)/server_state.h
COMMON_HDRS = $(COMMON_DIR)/common.h $(COMMON_DIR)/config.h $(COMMON_DIR)/ipc.h \
              $(COMMON_DIR)/messages.h $(COMMON_DIR)/types.h

# Executables
CLIENT_EXEC = client_app
SERVER_EXEC = server_app

# Default target
all: $(CLIENT_EXEC) $(SERVER_EXEC)

# Klient (používa ncurses)
$(CLIENT_EXEC): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS) $(COMMON_OBJS) $(LDFLAGS)

# Server (nepoužíva ncurses)
$(SERVER_EXEC): $(SERVER_OBJS) $(SIMULATION_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) $(SIMULATION_OBJS) $(COMMON_OBJS)

# Pravidlo pre kompiláciu .c súborov
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Vyčistenie
clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)
	find . -name "*.o" -type f -delete
	find . -name "*~" -type f -delete

# Pre účely ladenia/testovania
test: $(SERVER_EXEC) $(CLIENT_EXEC)
	@echo "Build complete. Run ./$(SERVER_EXEC) and ./$(CLIENT_EXEC) separately."

.PHONY: all clean test
