#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_SERVER_ADDR "127.0.0.1" //10.176.22.210, 10.10.88.214
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

typedef struct log_context {
    char filepath[128];
    FILE *file;
} log_ctx, *log_ctx_t;

log_ctx_t log_init(char *filepath, size_t pathlen);
void log_append(log_ctx_t ctx, char *msg, size_t msglen);
void log_destroy(log_ctx_t ctx);

// int cmp(const void *a , const void *b){
//     return *(double*)a > *(double*)b ? 1 : -1;
// }

#endif
