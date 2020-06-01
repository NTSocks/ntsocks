#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define DEFAULT_SERVER_ADDR "10.10.88.210"
#define DEFAULT_PORT 9091
#define SOCK_BACKLOG_CONN SOMAXCONN
#define MAX_NUM_THREADS 1024
#define DEFAULT_PAYLOAD_SIZE 16
#define NUM_REQ 1000000


#define	h_addr	h_addr_list[0]

int cmp(const void *a , const void *b){
    return *(double*)a > *(double*)b ? 1 : -1;
}

#endif