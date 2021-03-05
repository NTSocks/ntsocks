#ifndef COMMON_H
#define COMMON_H
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sched.h>
#include <errno.h>  // to show error code at Nread in server.c 
                    //  and Nwrite int client.c respectively
#include <unistd.h> // to get number of cpu core
#define DEFAULT_SERVER_ADDR "10.176.22.210"
#define DEFAULT_PORT 9091
#define SOCK_BACKLOG_CONN SOMAXCONN
#define MAX_NUM_THREADS 100
#define DEFAULT_PAYLOAD_SIZE 16
#define NUM_REQ 100000
#define RUN_TIME 3
#define MAX_TIME_IN_BANDWIDTH 10000
#define DEBUG 1

#define h_addr h_addr_list[0]

typedef struct log_context
{
    char filepath[128];
    FILE *file;
} log_ctx, *log_ctx_t;

typedef struct conn_ctx
{
    int sockfd;
    int id;
    int cpumask;
} conn_ctx;

log_ctx_t log_init(char *filepath, size_t pathlen);
void log_append(log_ctx_t ctx, char *msg, size_t msglen);
void log_destroy(log_ctx_t ctx);
void pin_1thread_to_1core(int core_id);
/**
 * @brief Get the core with odd or even id.
 * 
 * @param lastcore the last cpu core used.
 * @param cpucores the total cores in the machine
 * @param inc_or_dec 0, allocate from small to large.  
 *                      1, allocate from large to small.
 * @return int   the ID of the cpu core
 */
int get_core_id(int *last_core, int cpucores, int inc_or_dec);

/*
    get core from 16-31,48-63.
    when inc_or_dec is 0, allocate from small to large.
    when inc_or_dec is 1, allocate from large to small.
*/
int get_core_id2(int *last_core, int inc_or_dec);

/*
    get core from 0-15,32-47
    when inc_or_dec is 0, allocate from small to large.
    when inc_or_dec is 1, allocate from large to small.
*/
int get_core_id3(int *last_core, int inc_or_dec);

#endif
