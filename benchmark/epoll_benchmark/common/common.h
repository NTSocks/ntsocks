#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>

#define DEFAULT_SERVER_ADDR "127.0.0.1" //192.168.2.210, 10.10.88.214
#define DEFAULT_LOCALHOST "10.176.22.211"
#define DEFAULT_PORT 9091
#define SOCK_BACKLOG_CONN SOMAXCONN
#define CONNECT_NUMBER 1
#define DEFAULT_PAYLOAD_SIZE 16
#define NUM_REQ 100000
#define FDSIZE 10000  //must be greater than EPOLLEVENTS
#define EPOLLEVENTS 1000
#define BUFSIZE 1024

#define	h_addr	h_addr_list[0]

int cmp(const void *a , const void *b){
    return *(double*)a > *(double*)b ? 1 : -1;
}

#endif
