OBJ = main.o ntb-monitor.o shared_memory.o
#CXXFLAGS = -std=c++11 -O3
CCFLAGS = -std=c11 -O3 -Wall

ntb-monitor: ${OBJ}
			 gcc -o ntb-monitor ${OBJ} -lpthread -lrt

main.o: main.c constant.h ntb-monitor.h shared_memory.h log.h
ntb-monitor.o: ntb-monitor.c
shared_memory.o: shared_memory.c


install:
		cp ntb-monitor /usr/local/bin/ntb-monitor


clean:
		rm ntb-monitor *.o
			 

