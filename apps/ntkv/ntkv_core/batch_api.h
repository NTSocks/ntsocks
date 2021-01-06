#ifndef __BATCH_API_H__
#define __BATCH_API_H__
#include <stdint.h>
#include <sys/queue.h>
#include "mempool.h"
#include "common.h"

typedef uint16_t k_len;
typedef uint32_t d_len;
typedef uint8_t c_len;

enum BATCH_OP{
    BATCH_PUT,
    BATCH_GET,
    BATCH_WRITE_GET_REPLY
};

enum BATCH_STATUS{
    BATCH_OK,
    BATCH_EMPTY,
    BATCH_FULL,
    BATCH_ERR
};

struct data{
    k_len key_len;
    k_len value_len;
    uint8_t data[0];
}__attribute__ ((packed));
typedef struct data data_t;

struct batch_message{
    c_len cmd;
    d_len data_len;
    uint8_t data[0];
}__attribute__ ((packed));
typedef struct batch_message batch_message_t;

struct batch_reply{
    c_len err_no;
    d_len data_len;
    uint8_t data[0];
}__attribute__ ((packed));
typedef struct batch_reply batch_reply_t;

struct batch_node{
    k_len key_len;
    k_len value_len;
    void *data;
    TAILQ_ENTRY(batch_node) data_link;
}__attribute__ ((packed));
typedef struct batch_node batch_node_t;

typedef struct batch_data{
    mempool_t *mempool;
    int sockfd;
    d_len max_queue_size;
    d_len queue_size;
    d_len data_size;
    TAILQ_HEAD(, batch_node) queue_head;
}batch_data_t;

batch_data_t* batch_init(mempool_t *mempool, int sockfd, d_len queue_size);
int batch_push(batch_data_t *batch_queue, batch_node_t *item, int op, batch_reply_t *out);
batch_reply_t batch_write_get(batch_data_t *batch_queue);
batch_reply_t batch_write_put(batch_data_t *batch_queue);
void batch_write_get_reply(batch_data_t *batch_queue, int err_no);
static void batch_write(batch_data_t *batch_queue, int cmd);
batch_node_t* batch_pop(batch_data_t *batch_queue);
void batch_destroy(batch_data_t *batch_queue);

#endif