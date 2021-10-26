#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define DEFAULT_SERVER_ADDR "10.176.22.210" //10.176.22.210, 10.10.88.214
#define DEFAULT_PORT 9091
#define SOCK_BACKLOG_CONN SOMAXCONN
#define MAX_NUM_THREADS 500
#define DEFAULT_PAYLOAD_SIZE 16
#define NUM_REQ 100000
#define RUN_TIME 3

#define	h_addr	h_addr_list[0]

int cmp(const void *a , const void *b){
    return *(double*)a > *(double*)b ? 1 : -1;
}

#endif
