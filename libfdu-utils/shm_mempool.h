//
// Created by Bob Huang on 2020/3/26.
//

#ifndef TEMPLATECASE_SHM_MEMPOOL_H
#define TEMPLATECASE_SHM_MEMPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <stdbool.h>

#include "hash_map.h"

#define SMP_OK 0
#define SMP_MALLOC_FAIL -1
#define SMP_NOT_INIT -2

#define SMP_OVERFLOW_COLUMN 9999
#define SMP_SUFFIX "-mp"
#define MUTEX_SEM_SUFFIX "-mutex"

#define SHM_PATH_PREFIX "/dev/shm"

#define SHM_OFFSET_SIZE 32
#define NODE_IDX_SIZE sizeof(int)

typedef struct
{
        int column;
        int node_idx;
        uint64_t shm_offset; // point to the pre-applied
                             //  shm data memory segment
        int next_node_idx;   // indicate the index of
                             //  next shm_mempool_node
} __attribute__((packed)) shm_mempool_node;

typedef struct
{
        int column;
        int total_count;
        int used_count;
        int block_len;
        int free_header_idx;
        int free_tail_idx;
        int used_header_idx;

} __attribute__((packed)) shm_mempool;
typedef shm_mempool *shm_mempool_t;

typedef struct
{
        char *shm_mp_name;
        size_t shm_mp_name_len;
        int shm_mp_fd;

        void *base_shm_mp;
        shm_mempool *shm_mp;
        shm_mempool_node *shm_mp_nodes;

        sem_t *sem_mutex;
        char *sem_mutex_name;
        size_t sem_mutex_name_len;

        char *shm_mem_name;
        size_t shm_mem_name_len;
        int shm_mem_fd;
        char *shm_mem;

        /**
         * @brief  hold <shm segment addr offset, shm mp node_idx>
         * @note   key: shm segment virtual memory addr offset,  
         *         value: shm mp node index
         * @retval None
         */
        HashMap mp_node_map;
        int *node_idxs;
        char **offset_strs;

} __attribute__((packed)) shm_mp_handler;
typedef shm_mp_handler *shm_mp_handler_t;

shm_mp_handler_t shm_mp_init(
    size_t block_len, size_t block_count,
    char *shm_name, size_t shm_name_len);

shm_mempool_node *shm_mp_malloc(
    shm_mp_handler_t mp_handler, size_t size);

shm_mempool_node *shm_mp_node(
    shm_mp_handler_t mp_handler, size_t node_idx);

shm_mempool_node *shm_mp_node_by_shmaddr(
    shm_mp_handler_t mp_handler, char *shmaddr);

char *shm_offset_mem(
    shm_mp_handler_t mp_handler, size_t node_idx);

int shm_mp_free(
    shm_mp_handler_t mp_handler, shm_mempool_node *node);

int shm_mp_destroy(
    shm_mp_handler_t mp_handler, int is_unlink);

int shm_mp_runtime_print(shm_mp_handler_t mp_handler);

#endif //TEMPLATECASE_SHM_MEMPOOL_H
