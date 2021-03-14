CROSS = 
CC = $(CROSS)gcc
CXX = $(CROSS)g++
DEBUG = -g -O2 #-DENABLE_DEBUG
CFLAGS = $(DEBUG) -Wall -std=c11 -c -D_DEFAULT_SOURCE -DHAVE_GCC_C11_ATOMICS
TARGET_CFLAGS = $(DEBUG) -Wall -std=c11 -D_DEFAULT_SOURCE -DHAVE_GCC_C11_ATOMICS
TEST_TARGET_CPPFLAGS = $(DEBUG) -std=c++11 -Wall -D_DEFAULT_SOURCE -DHAVE_GCC_C11_ATOMICS 

CPPFLAGS = -std=c++11 -Wall
RM = rm -rf

LIB_DIR = lib
TEST_DIR = tests

SRCS = $(wildcard src/**/*.c src/*.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))

TEST_SRCS = $(wildcard $(TEST_DIR)/**/*.cpp $(TEST_DIR)/*.cpp)
TEST_OBJS = $(patsubst %.cpp, %.o, $(TEST_SRCS))

INCLUDE_PATH = -I./include/ -I./src/include/ -I./src/utils/include -I/usr/local/include 
LIB_PATH = -L./lib/ -L/usr/local/lib 

LIBS = -lpthread -lrt -luuid -lfdu-utils
#LIBS = -ltest
#LIBS = -libtest.a

VERSION = 1.0.0
TARGET = bin/ntb-monitor
TEST_TARGET = run_test


#main.o: main.c constant.h ntb-monitor.h shared_memory.h log.h
#			$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH)
SRC_LIST = src/ntm_shmring.c  	\
		   src/ntm_shm.c      	\
		   src/nts_shmring.c	\
		   src/nts_shm.c		\
		   src/ntb_monitor.c 

$(TARGET): ${OBJS}
			mkdir -p bin
			$(CC) $^ -o $@ $(LIB_PATH) $(LIBS) $(TARGET_CFLAGS)
$(TEST_TARGET): $(TEST_SRCS) $(SRC_LIST)
			@$(CXX) $(CPPFLAGS) $(INCLUDE_PATH) -o $(TEST_TARGET) $^ $(LIBS) $(TEST_TARGET_CPPFLAGS)
			@./$(TEST_TARGET)

$(OBJS):%.o : %.c
			$(CC) $(CFLAGS) $< -o $@ $(INCLUDE_PATH) $(LIBS)
%.o : %.cpp
			$(CXX) $(CPPFLAGS) $< -o $@ $(INCLUDE_PATH)	$(LIBS)	

		
all: $(TARGET) $(TEST_TARGET)
test: $(TEST_TARGET)

install:
		cp ntb-monitor /usr/local/bin/ntb-monitor


clean:
		$(RM) $(TARGET) $(TEST_TARGET) $(OBJS) $(TEST_OBJS)
			 

