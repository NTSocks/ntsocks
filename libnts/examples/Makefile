CROSS = 
CC = $(CROSS)gcc
CXX = $(CROSS)g++
DEBUG = -g -O2
CFLAGS += $(DEBUG) -Wall -std=c11 -c -D_BSD_SOURCE -DHAVE_GCC_C11_ATOMICS -DENABLE_DEBUG
CFLAGS += -I/usr/local/include 
RM = rm -rf
BIN_PATH=./bin

EP_SERVER_TARGET=epoll_server
EP_CLIENT_TARGET=epoll_client

EP_SERVER_SRCS=nts_epoll_echo_server.c 
EP_CLIENT_SRCS=nts_epoll_echo_client.c

LIBS+= -lpthread -lrt
LDFLAGS+= $(LIBS)

TARGET=epoll_server epoll_client

all: clean $(TARGET)
	$(RM) *.o 
	mkdir -p $(BIN_PATH)
	mv $(TARGET) $(BIN_PATH)

$(EP_SERVER_TARGET): $(EP_SERVER_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) 

$(EP_CLIENT_TARGET): $(EP_CLIENT_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) 

clean: 
	$(RM) *.o $(TARGET) $(BIN_PATH)
