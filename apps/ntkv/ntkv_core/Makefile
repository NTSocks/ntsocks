CC = gcc
CPP = g++
CFLAGS = -c -g -lpthread
LDFLAGS = -lpthread
SERVER_OBJS = 	server.o\
				linked_hashmap.o\
				mempool.o \
				batch_api.o \
				common.o \
				measure.o \
				server_api.o

SERVER_OUT = server

CLIENT_OBJS = 	client.o\
				linked_hashmap.o\
				mempool.o \
				batch_api.o \
				client_api.o \
				common.o \
				measure.o 

CLIENT_OUT = client

UNIT_TEST_DIR = unit_test
LINKED_HASHMAP_OBJS = 	linked_hashmap_test.o\
			 			linked_hashmap.o\
						mempool.o
LINKED_HASHMAP_OUT = linked_hashmap_test

MEMPOOL_OBJS =	mempool_test.o\
				mempool.o
MEMPOOL_OUT = mempool_test

all: SERVER_OUT CLIENT_OUT LINKED_HASHMAP_OUT MEMPOOL_OUT move

move:
	mkdir -p bin/
	mv *.o $(SERVER_OUT) $(CLIENT_OUT) $(LINKED_HASHMAP_OUT) $(MEMPOOL_OUT) bin/

SERVER_OUT : $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $(SERVER_OUT) $(LDFLAGS)

CLIENT_OUT : $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o $(CLIENT_OUT) $(LDFLAGS)

LINKED_HASHMAP_OUT : $(LINKED_HASHMAP_OBJS)
	$(CC) $(LINKED_HASHMAP_OBJS) -o $(LINKED_HASHMAP_OUT) $(LDFLAGS)

MEMPOOL_OUT : $(MEMPOOL_OBJS)
	$(CC) $(MEMPOOL_OBJS) -o $(MEMPOOL_OUT) $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) $^ -o $@

%.o : $(UNIT_TEST_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

clean: 
	rm -rf *.o bin/* bin
