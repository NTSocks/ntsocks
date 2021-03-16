BIN_PATH=./bin
INC_PATH=./include
LIB_PATH=./libs
LIB_NAME=fdu-utils
LIB_OBJS=utils.o shm_mempool.o shmring.o hash_map.o \
			array.o ntp2nts_msg.o ntp2nts_shm.o		\
			epoll_shm.o sem_shmring.o 				\
			epoll_sem_shm.o epoll_event_queue.o
LIB_SRCS=utils.c shm_mempool.c shmring.c hash_map.c \
			array.c ntp2nts_msg.c ntp2nts_shm.c		\
			epoll_shm.c sem_shmring.c 				\
			epoll_sem_shm.c epoll_event_queue.c

CC=gcc
LIBS+= -lpthread -lrt -luuid
RELEASE= -DNDEBUG # disable assert
DEBUG = -g -O2 #-DENABLE_DEBUG
CFLAGS += $(RELEASE) $(DEBUG) -Wall -std=c11 -D_DEFAULT_SOURCE -DHAVE_GCC_C11_ATOMICS
LDFLAGS+= -L$(LIB_PATH) -l$(LIB_NAME)

TARGET=shmring ntp2nts-shm sem_shmring hashmap_main ep_eq_main ep_eqserver ep_eqclient \
			bulk_shmring multi_thread_bulk_shmring ntp2nts-bulk-shm

all: clean $(LIB_OBJS) fdu_utils_static_lib fdu_utils_shared_lib local-install shmring ntp2nts-shm \
		sem_shmring hashmap_main ep_eq_main ep_eqserver ep_eqclient \
		bulk_shmring multi_thread_bulk_shmring ntp2nts-bulk-shm
	rm -f *.o 
	mv $(TARGET) $(BIN_PATH)
	cp lib$(LIB_NAME).* $(LIB_PATH)
	cp *.h $(INC_PATH)


shmring: shmring_main.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)
	# mv $(TARGET) $(BIN_PATH)

bulk_shmring: shmring_bulk_main.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)

multi_thread_bulk_shmring: shmring_bulk_multi_thread_main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)

ntp2nts-shm: shm_main.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)
	# mv $(TARGET) $(BIN_PATH)

sem_shmring: sem_shmring_main.c	
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)

hashmap_main: hashmap_main.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)

ep_eq_main: ep_eventqueue_main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)

ep_eqserver: ep_eqserver_main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)

ep_eqclient: ep_eqclient_main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)

ntp2nts-bulk-shm: shm_bulk_main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -I$(INC_PATH)



fdu_utils_static_lib: $(LIB_OBJS)
	ar cq lib$(LIB_NAME).a $^ 

fdu_utils_shared_lib:
	$(CC) -fPIC -shared $(CFLAGS) $(LIB_SRCS) -o lib$(LIB_NAME).so $(LIBS) 

$(LIB_OBJS): $(LIB_SRCS)
	$(CC) $(CFLAGS)  $(LIBS) -c -o $@ $<


clean: 
	rm -rf *.o lib$(LIB_NAME).* $(TARGET) $(BIN_PATH)/* ${LIB_PATH}/*

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
	cp *.h /usr/local/include
