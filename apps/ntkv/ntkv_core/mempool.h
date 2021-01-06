#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "kv_log.h"

#define mem_size unsigned long long
#define KB (mem_size)(1 << 10)
#define MB (mem_size)(1 << 20)
#define GB (mem_size)(1 << 30)

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);
struct node{
    // node size including header and tailer
    mem_size node_size;
    uint8_t is_free;
    struct node *pre, *next;
}node;
typedef struct node node_t;

#define NODE_HEADER_SIZE sizeof(node_t)
#define NODE_TAILER_SIZE sizeof(node_t*)

struct chunk{
    void *start;
    unsigned int id;
    // 
    mem_size mempool_size;
    // current chunk size that are allocated
    mem_size alloc_chunk_size;
    // current chunk total data size that are allocated
    mem_size alloc_data_size;
    node_t *free_list, *alloc_list;
    struct chunk *next;
}chunk;
typedef struct chunk chunk_t;

struct mempool{
    unsigned int last_id;
    uint8_t auto_extend, need_lock;
    mem_size each_chunk_size, total_chunk_size, max_mempool_size;
    pthread_mutex_t lock;
    chunk_t *chunk_list;
}mempool;
typedef struct mempool mempool_t;

mempool_t* mempool_init(mem_size each_chunk_size, mem_size max_pool_size, uint8_t need_lock);
void* mempool_alloc(mempool_t *mempool, mem_size node_data_size);
void mempool_free(mempool_t *mempool, void *addr);
mempool_t* mempool_clear(mempool_t *mempool);
int mempool_destroy(mempool_t *mempool);

mem_size get_total_mempool_size(mempool_t *mempool);
mem_size get_used_alloc_chunk_size(mempool_t *mempool);
mem_size get_used_alloc_data_size(mempool_t *mempool);
mem_size get_chunk_count(mempool_t *mempool);
mem_size get_alloc_node_count(mempool_t *mempool);
mem_size get_free_node_count(mempool_t *mempool);
void print_mempool(mempool_t *mempool);

#endif
