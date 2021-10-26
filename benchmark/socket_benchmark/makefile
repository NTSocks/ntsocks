CC = gcc
CFLAGS = -c -Wall -pthread -lm -lrt
LDFLAGS = -pthread -lm -lrt
TARGETS = client server
CLIENT_OBJS = client.o
SERVER_OBJS = server.o
CLIENT_DIR = socket-client
SERVER_DIR = socket-server
BIN_DIR = bin

all: $(TARGETS) move

move:
	mkdir -p $(BIN_DIR)
	mv *.o $(TARGETS) $(BIN_DIR)

server: $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o server $(LDFLAGS)

client: $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o client $(LDFLAGS)

%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BIN_DIR)/*
