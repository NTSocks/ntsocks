BIN_PATH=./bin
INC_PATH=./include
LIB_PATH=./libs
LIB_NAME=fdu-utils
LIB_OBJS=shm_mempool.o shmring.o 
LIB_SRCS=shm_mempool.c shmring.c

CC=gcc
LIBS+= -lpthread -lrt
DEBUG = -g -O2
CFLAGS += $(DEBUG) -Wall -std=c11 -D_BSD_SOURCE -DHAVE_GCC_C11_ATOMICS 
LDFLAGS+= -L$(LIB_PATH) -l$(LIB_NAME)

TARGET=shmring 

all: clean $(LIB_OBJS) fdu_utils_static_lib fdu_utils_shared_lib local-install $(TARGET)
	rm -f *.o 


shmring: shmring_main.c 
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@ $(LDFLAGS) -I$(INC_PATH)
	mv $(TARGET) $(BIN_PATH)

fdu_utils_static_lib: $(LIB_OBJS)
	ar cq lib$(LIB_NAME).a $^ 

fdu_utils_shared_lib:
	$(CC) -fPIC -shared $(CFLAGS) $(LIBS) $(LIB_SRCS) -o lib$(LIB_NAME).so 

$(LIB_OBJS): $(LIB_SRCS)
	$(CC) $(CFLAGS)  $(LIBS) -c -o $@ $<


clean: 
	rm -rf *.o lib$(LIB_NAME).* $(TARGET) 

distclean:
	rm -rf $(LIB_PATH)
	rm -rf $(TARGET)
	rm -rf $(INC_PATH)
	rm -rf $(BIN_PATH)


local-install:
	mkdir -p $(LIB_PATH)
	mkdir -p $(BIN_PATH)
	mkdir -p $(INC_PATH)
	cp lib$(LIB_NAME).* $(LIB_PATH)
	cp *.h $(INC_PATH)

install:
	cp lib$(LIB_NAME).* /usr/local/lib
