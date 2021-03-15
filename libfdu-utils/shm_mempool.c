//
// Created by Bob Huang on 2020/3/26.
//

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <assert.h>

#include "shm_mempool.h"
#include "utils.h"
#include "nt_log.h"

#ifdef ENABLE_DEBUG
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);
#else
DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);
#endif //ENABLE_DEBUG

shm_mp_handler_t
shm_mp_init(size_t block_len,
            size_t block_count, char *shm_name, size_t shm_name_len)
{

    int shm_mp_size;
    bool is_exist;
    int ret;
    shm_mp_handler_t mp_handler;

    mp_handler = (shm_mp_handler_t)(malloc(sizeof(shm_mp_handler)));
    if (mp_handler == NULL)
    {
        return NULL;
    }

    memset(mp_handler, 0, sizeof(shm_mp_handler));

    mp_handler->shm_mem_name_len = shm_name_len + 1;
    mp_handler->shm_mem_name = (char *)
        malloc(mp_handler->shm_mem_name_len);
    if (mp_handler->shm_mem_name == NULL)
    {
        perror("malloc");
        goto FAIL;
    }

    // sprintf(mp_handler->shm_mem_name, "%s", shm_name);
    memset(mp_handler->shm_mem_name,
           0, mp_handler->shm_mem_name_len);
    memcpy(mp_handler->shm_mem_name,
           shm_name, shm_name_len);
    DEBUG("shm_name=%s, shm_mem_name=%s, sizeof shm_mem_name = %ld, size of shm_name = %ld\n",
           shm_name, mp_handler->shm_mem_name,
           sizeof(mp_handler->shm_mem_name), sizeof(shm_name));

    mp_handler->shm_mp_name_len =
        shm_name_len + strlen(SMP_SUFFIX) + 1;
    mp_handler->shm_mp_name = (char *)
        malloc(mp_handler->shm_mp_name_len);
    if (mp_handler->shm_mp_name == NULL)
    {
        perror("malloc");
        goto FAIL;
    }

    memset(mp_handler->shm_mp_name,
           0, mp_handler->shm_mp_name_len);
    sprintf(mp_handler->shm_mp_name,
            "%s%s", shm_name, SMP_SUFFIX);
    DEBUG("shm_mp_name=%s \n", mp_handler->shm_mp_name);

    mp_handler->sem_mutex_name_len =
        shm_name_len + strlen(MUTEX_SEM_SUFFIX) + 1;
    mp_handler->sem_mutex_name = (char *)
        malloc(mp_handler->sem_mutex_name_len);
    if (mp_handler->sem_mutex_name == NULL)
    {
        perror("malloc");
        goto FAIL;
    }

    memset(mp_handler->sem_mutex_name,
           0, mp_handler->sem_mutex_name_len);
    sprintf(mp_handler->sem_mutex_name,
            "%s%s", mp_handler->shm_mem_name, MUTEX_SEM_SUFFIX);
    DEBUG("sem_mutex_name=%s \n", mp_handler->sem_mutex_name);

    char tmp_shm[50];
    memset(tmp_shm, 0, sizeof(tmp_shm));
    sprintf(tmp_shm, "%s%s",
            SHM_PATH_PREFIX, mp_handler->shm_mem_name);

    is_exist = false;
    if (!access(tmp_shm, R_OK | W_OK))
        is_exist = true;

    mp_handler->shm_mem_fd = shm_open(
        mp_handler->shm_mem_name, O_CREAT | O_RDWR, 0666);
    if (mp_handler->shm_mem_fd == -1)
    {
        perror("shm_open");
        goto FAIL;
    }
    // set the permission of shm fd for write/read in non-root users
    fchmod(mp_handler->shm_mem_fd, 0666);

    if (!is_exist)
    {
        ret = ftruncate(mp_handler->shm_mem_fd,
                        sizeof(char) * block_count * block_len);
        if (ret == -1)
        {
            perror("ftruncate");
            goto FAIL;
        }
    }

    mp_handler->shm_mem = (char *)(mmap(NULL,
                                        sizeof(char) * block_count * block_len,
                                        PROT_READ | PROT_WRITE, MAP_SHARED,
                                        mp_handler->shm_mem_fd, 0));
    if (mp_handler->shm_mem == MAP_FAILED)
    {
        perror("mmap");
        goto FAIL;
    }
    close(mp_handler->shm_mem_fd);

    memset(tmp_shm, 0, sizeof(tmp_shm));
    sprintf(tmp_shm, "%s%s",
            SHM_PATH_PREFIX, mp_handler->shm_mp_name);

    is_exist = false;
    if (!access(tmp_shm, R_OK | W_OK))
        is_exist = true;

    mp_handler->shm_mp_fd = shm_open(
        mp_handler->shm_mp_name, O_CREAT | O_RDWR, 0666);
    if (mp_handler->shm_mp_fd == -1)
    {
        perror("shm_open");
        goto FAIL;
    }
    // set the permission of shm fd for write/read in non-root users
    fchmod(mp_handler->shm_mp_fd, 0666);

    shm_mp_size = sizeof(shm_mempool) + sizeof(shm_mempool_node) * block_count;
    if (!is_exist)
    {
        ret = ftruncate(mp_handler->shm_mp_fd, shm_mp_size);
        if (ret == -1)
        {
            perror("ftruncate");
            goto FAIL;
        }
    }

    DEBUG("mmap base_shm_mp with shm_mp_size=%d \n", shm_mp_size);
    mp_handler->base_shm_mp = mmap(NULL, shm_mp_size,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   mp_handler->shm_mp_fd, 0);
    if (mp_handler->base_shm_mp == MAP_FAILED)
    {
        perror("mmap");
        goto FAIL;
    }
    close(mp_handler->shm_mp_fd);

    mp_handler->shm_mp = (shm_mempool *)
                             mp_handler->base_shm_mp;
    mp_handler->shm_mp_nodes = (shm_mempool_node *)((char *)mp_handler->base_shm_mp + sizeof(shm_mempool));

    // store old, umask for world-writable access of semaphores
    mode_t old_umask = umask(0);
    mp_handler->sem_mutex = sem_open(
        mp_handler->sem_mutex_name, O_CREAT, 0666, 0);
    if (mp_handler->sem_mutex == SEM_FAILED)
    {
        perror("sem_open: sem_mutex");
        goto FAIL;
    }
    // restore old mask
    umask(old_umask);

    // if first create shm mempool, init
    if (!is_exist)
    {
        DEBUG("first create shm mempool \n");

        memset(mp_handler->shm_mp, 0, sizeof(shm_mempool));
        memset(mp_handler->shm_mp_nodes,
               0, sizeof(shm_mempool_node) * block_count);

        mp_handler->shm_mp->column = 1;
        mp_handler->shm_mp->total_count = block_count;
        mp_handler->shm_mp->used_count = 0;
        mp_handler->shm_mp->block_len = block_len;
        mp_handler->shm_mp->free_header_idx = -1;
        mp_handler->shm_mp->free_tail_idx = -1;
        mp_handler->shm_mp->used_header_idx = -1;
        DEBUG("mp_handler->shm_mp: column=%d, total_count=%d, block_len=%d \n",
              mp_handler->shm_mp->column, mp_handler->shm_mp->total_count,
              mp_handler->shm_mp->block_len);

        shm_mempool_node *curr_node = NULL;
        shm_mempool_node *new_node = NULL;
        uint64_t shm_offset = 0;
        DEBUG("init shm mempool nodes\n");
        for (int i = 0; i < mp_handler->shm_mp->total_count; ++i)
        {
            new_node = &(mp_handler->shm_mp_nodes[i]);
            new_node->column = 1;
            new_node->shm_offset = shm_offset;
            new_node->next_node_idx = -1;
            new_node->node_idx = i;

            if (i == 0)
            {
                mp_handler->shm_mp->free_header_idx = new_node->node_idx;
                mp_handler->shm_mp->free_tail_idx = new_node->node_idx;
                curr_node = new_node;
            }
            else
            {
                curr_node->next_node_idx = new_node->node_idx;
                mp_handler->shm_mp->free_tail_idx = new_node->node_idx;
                // mp_handler->shm_mp_nodes[curr_node->next_node_idx]
                curr_node = new_node;
            }

            shm_offset += mp_handler->shm_mp->block_len;
        }
    }

    mp_handler->offset_strs = (char **)
        malloc(sizeof(char *) * block_count);
    for (size_t i = 0; i < block_count; i++)
    {
        mp_handler->offset_strs[i] = (char *)
            malloc(sizeof(char) * SHM_OFFSET_SIZE);
        memset(mp_handler->offset_strs[i], 0, SHM_OFFSET_SIZE);
    }

    /**
     *  init hash map for the mapping between shm segment
     *      virtual memory addr and shm mp node index 
     */
    mp_handler->mp_node_map = createHashMap(NULL, NULL);
    mp_handler->node_idxs = (int *)malloc(sizeof(int) * block_count);
    char *tmp_mem = mp_handler->shm_mem;
    for (size_t i = 0; i < block_count; i++)
    {
        mp_handler->node_idxs[i] = i;
        sprintf(mp_handler->offset_strs[i], "%ld",
                mp_handler->shm_mp_nodes[i].shm_offset);
        Put(mp_handler->mp_node_map,
            mp_handler->offset_strs[i], &mp_handler->node_idxs[i]);

        tmp_mem += block_len;
    }

    ret = sem_post(mp_handler->sem_mutex);
    if (ret == -1)
    {
        perror("sem_post: sem_mutex");
        goto FAIL;
    }

    DEBUG("shm_mp_init success\n");
    return mp_handler;

FAIL:

    if (mp_handler)
    {
        if (mp_handler->shm_mp_name)
        {
            free(mp_handler->shm_mp_name);
            mp_handler->shm_mp_name = NULL;
        }

        if (mp_handler->shm_mem_name)
        {
            free(mp_handler->shm_mem_name);
            mp_handler->shm_mem_name = NULL;
        }

        if (mp_handler->sem_mutex_name)
        {
            free(mp_handler->sem_mutex_name);
            mp_handler->sem_mutex_name = NULL;
        }

        if (mp_handler->mp_node_map)
        {
            Clear(mp_handler->mp_node_map);
            free(mp_handler->mp_node_map);
            mp_handler->mp_node_map = NULL;
        }

        if (mp_handler->node_idxs)
        {
            free(mp_handler->node_idxs);
            mp_handler->node_idxs = NULL;
        }

        if (mp_handler->shm_mp_fd > 0)
        {
            close(mp_handler->shm_mp_fd);
        }

        if (mp_handler->shm_mem_fd)
        {
            close(mp_handler->shm_mem_fd);
        }

        free(mp_handler);
        mp_handler = NULL;
    }

    return NULL;
}

shm_mempool_node *
shm_mp_malloc(shm_mp_handler_t mp_handler, size_t size)
{
    assert(mp_handler);
    DEBUG("shm_mp_malloc start.");

    DEBUG("mp_handler->shm_mp: column=%d, total_count=%d, block_len=%d \n",
          mp_handler->shm_mp->column, mp_handler->shm_mp->total_count,
          mp_handler->shm_mp->block_len);

    shm_mempool_node *node;
    size = mp_handler->shm_mp->block_len;

    int node_idx = mp_handler->shm_mp->free_header_idx;
    if (UNLIKELY(node_idx == -1))
    {
        ERR("no free shm-mempool node with free_header_idx=%d, free_tail_idx=%d",
            mp_handler->shm_mp->free_header_idx,
            mp_handler->shm_mp->free_tail_idx);
        shm_mp_runtime_print(mp_handler);
        return NULL;
    }

    node = &mp_handler->shm_mp_nodes[node_idx];
    DEBUG("node->node_idx=%d, node_idx=%d", node->node_idx, node_idx);
    mp_handler->shm_mp->free_header_idx = node->next_node_idx;
    __WRITE_BARRIER__;

    if (node->next_node_idx == -1)
    {
        printf("mp_handler->shm_mp->free_header_idx == -1, node_idx=%d\n", node->node_idx);
    }

    // TODO:
    // node->next_node_idx = mp_handler->shm_mp->used_header_idx;
    // mp_handler->shm_mp->used_header_idx = node->node_idx;

    DEBUG("shm_mp_malloc success\n");

    return node;
}

shm_mempool_node *
shm_mp_node(shm_mp_handler_t mp_handler, size_t node_idx)
{
    if (node_idx < mp_handler->shm_mp->total_count)
    {
        return &mp_handler->shm_mp_nodes[node_idx];
    }

    return NULL;
}

shm_mempool_node *
shm_mp_node_by_shmaddr(shm_mp_handler_t mp_handler, char *shmaddr)
{
    assert(shmaddr);

    int *node_idx;
    uint64_t offset = shmaddr - mp_handler->shm_mem;
    DEBUG("\n offset=%ld\n", offset);
    char offset_str[SHM_OFFSET_SIZE] = {0};
    sprintf(offset_str, "%ld", offset);

    node_idx = (int *)Get(mp_handler->mp_node_map, offset_str);
    if (node_idx)
    {
        DEBUG("[shm_mp_node_by_shmaddr] node_idx=%d \n", *node_idx);
        return &mp_handler->shm_mp_nodes[*node_idx];
    }

    return NULL;
}

char *shm_offset_mem(
    shm_mp_handler_t mp_handler, size_t node_idx)
{
    assert(mp_handler);

    DEBUG("shm_offset_mem start. node_idx = %d, total_count = %d\n",
          node_idx, mp_handler->shm_mp->total_count);

    if (LIKELY(node_idx >= 0 && node_idx < mp_handler->shm_mp->total_count))
    {
        DEBUG("shm_offset_mem success [node_idx=%d].\n", node_idx);
        return (mp_handler->shm_mem +
                mp_handler->shm_mp_nodes[node_idx].shm_offset);
    }

    DEBUG("shm_offset_mem failed. node_idx = %d, total_count = %d\n",
          node_idx, mp_handler->shm_mp->total_count);

    return NULL;
}

int shm_mp_free(
    shm_mp_handler_t mp_handler, shm_mempool_node *node)
{
    assert(mp_handler);
    assert(node);

    DEBUG("shm_mp_free start.");
    DEBUG("mp_handler->shm_mp: column=%d, total_count=%d, block_len=%d \n",
          mp_handler->shm_mp->column,
          mp_handler->shm_mp->total_count,
          mp_handler->shm_mp->block_len);

    // TODO:
    if (mp_handler->shm_mp->free_tail_idx != -1)
    {
        DEBUG("start update free_tail_idx=%d (!= -1), free_header_idx=%d",
              mp_handler->shm_mp->free_tail_idx,
              mp_handler->shm_mp->free_header_idx);

        if (node->node_idx == -1)
        {
            fprintf(stderr, "mp_free node->node_idx == -1 \n");
        }

        mp_handler->shm_mp_nodes[mp_handler->shm_mp->free_tail_idx]
            .next_node_idx = node->node_idx;

        mp_handler->shm_mp->free_tail_idx = node->node_idx;
        __WRITE_BARRIER__;
    }
    else
    {
        DEBUG("free_tail_idx == -1, free_header_idx=%d",
              mp_handler->shm_mp->free_header_idx);
        DEBUG("free_tail_idx=%d, free_header_idx=%d, node_idx=%d\n",
              mp_handler->shm_mp->free_tail_idx,
              mp_handler->shm_mp->free_header_idx,
              node->node_idx);
        node->next_node_idx = -1;
        mp_handler->shm_mp->free_tail_idx = node->node_idx;
        mp_handler->shm_mp->free_header_idx = node->node_idx;
        __WRITE_BARRIER__;
    }

    INFO("shm_mp_free success.\n");

    return SMP_OK;
}

int shm_mp_destroy(
    shm_mp_handler_t mp_handler, int is_unlink)
{
    assert(mp_handler);
    DEBUG("shm_mp_destroy start.\n");
    DEBUG("mp_handler->shm_mp: column=%d, total_count=%d, block_len=%d \n",
          mp_handler->shm_mp->column,
          mp_handler->shm_mp->total_count,
          mp_handler->shm_mp->block_len);

    int block_count = 0;
    int block_len, shm_mp_size;

    if (mp_handler->shm_mp || mp_handler->shm_mem)
    {
        DEBUG("munmap, close fd and sem_close start.\n");
        block_count = mp_handler->shm_mp->total_count;
        block_len = mp_handler->shm_mp->block_len;
        shm_mp_size = sizeof(shm_mempool) + sizeof(shm_mempool_node) * block_count;

        DEBUG("munmap and close base_shm_mp (shm mempool), shm_mp_size=%d, block_count=%d\n",
              shm_mp_size, block_count);
        munmap(mp_handler->base_shm_mp, (size_t)(shm_mp_size));

        DEBUG("munmap and close shm_mem start. \n");
        munmap(mp_handler->shm_mem, (size_t)(block_count * block_len));

        DEBUG("sem close sem_mutex\n");
        sem_close(mp_handler->sem_mutex);

        if (is_unlink)
        {
            DEBUG("unlink shm start with shm name=%s.\n",
                  mp_handler->shm_mem_name);
            shm_unlink(mp_handler->shm_mp_name);
            shm_unlink(mp_handler->shm_mem_name);
            sem_unlink(mp_handler->sem_mutex_name);
        }
    }

    if (mp_handler->shm_mp_name)
    {
        free(mp_handler->shm_mp_name);
        mp_handler->shm_mp_name = NULL;
    }

    if (mp_handler->shm_mem_name)
    {
        free(mp_handler->shm_mem_name);
        mp_handler->shm_mem_name = NULL;
    }

    if (mp_handler->sem_mutex_name)
    {
        free(mp_handler->sem_mutex_name);
        mp_handler->sem_mutex_name = NULL;
    }

    if (mp_handler->mp_node_map)
    {
        Clear(mp_handler->mp_node_map);
        free(mp_handler->mp_node_map);
        mp_handler->mp_node_map = NULL;
    }

    if (mp_handler->node_idxs)
    {
        free(mp_handler->node_idxs);
        mp_handler->node_idxs = NULL;
    }

    if (block_count > 0 && mp_handler->offset_strs)
    {
        for (size_t i = 0; i < block_count; i++)
        {
            free(mp_handler->offset_strs[i]);
        }
        free(mp_handler->offset_strs);
    }

    free(mp_handler);
    mp_handler = NULL;
    DEBUG("shm_mp_destroy success \n");

    return SMP_OK;
}

int shm_mp_runtime_print(shm_mp_handler_t mp_handler)
{
    DEBUG("mp_handler->shm_mp: column=%d, total_count=%d, block_len=%d \n",
          mp_handler->shm_mp->column,
          mp_handler->shm_mp->total_count,
          mp_handler->shm_mp->block_len);

    if (mp_handler == NULL || mp_handler->shm_mp == NULL)
    {
        DEBUG("shm memory pool not init yet! \n");
        return SMP_NOT_INIT;
    }

    printf("\n*********************** memory pool runtime report start************************\n");
    printf("pool no[%d] blocksize[%d] blockTotalCount[%d]\n",
           mp_handler->shm_mp->column, mp_handler->shm_mp->block_len,
           mp_handler->shm_mp->total_count);

    printf("*********************** memory pool runtime report end**************************\n");

    return SMP_OK;
}
