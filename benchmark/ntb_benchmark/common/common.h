#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_SERVER_ADDR "10.176.22.210"
#define DEFAULT_PORT 9091
#define SOCK_BACKLOG_CONN SOMAXCONN
#define MAX_NUM_THREADS 1024
#define DEFAULT_PAYLOAD_SIZE 16
#define NUM_REQ 100000
#define RUN_TIME 3

#define	h_addr	h_addr_list[0]

typedef struct log_context {
    char filepath[128];
    FILE *file;
} log_ctx, *log_ctx_t;

log_ctx_t log_init(char *filepath, size_t pathlen);
void log_append(log_ctx_t ctx, char *msg, size_t msglen);
void log_destroy(log_ctx_t ctx);

#endif
