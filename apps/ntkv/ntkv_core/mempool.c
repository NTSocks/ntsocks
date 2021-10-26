#include "mempool.h"

#define ALIGN_SIZE(n) (n + sizeof(long) - ((sizeof(long) - 1) & n))

#define node_delete(head, x)                      \
    do {                                          \
        if (!x->pre) {                           \
            head = x->next;                       \
            if (x->next) x->next->pre = NULL;    \
        } else {                                  \
            x->pre->next = x->next;              \
            if (x->next) x->next->pre = x->pre; \
        }                                         \
    } while (0)

#define node_insert(head, x)          \
    do {                              \
        x->pre = NULL;               \
        x->next = head;               \
        if (head) head->pre = x;     \
        head = x;                     \
    } while (0)

static void init_chunk(void *chunk_head, mem_size chunk_size);
static void merge_free_node(mempool_t *mempool, chunk_t *chunk, node_t *node);
static chunk_t* extend_chunk_list(mempool_t *mempool, mem_size chunk_size);

mempool_t* mempool_init(mem_size each_chunk_size, mem_size max_pool_size, uint8_t need_lock){
    if (each_chunk_size > max_pool_size){
        fprintf(stderr, "Init mempool failed. each chunk size cannot be larger than max pool size\n");
        return NULL;
    }
    mempool_t *mempool = (mempool_t*) malloc(sizeof(mempool_t));
    if (mempool == NULL){
        fprintf(stderr, "malloc memory for mempool failed\n");
        return NULL;
    }
    DEBUG("NODE HEADER SIZE is %llu, NODE TAILER SIZE is %llu", NODE_HEADER_SIZE, NODE_TAILER_SIZE);
    mempool->auto_extend = each_chunk_size < max_pool_size ? 1 : 0;
    mempool->need_lock = need_lock;
    if (need_lock) pthread_mutex_init(&mempool->lock, NULL);
    mempool->last_id = 0;
    mempool->each_chunk_size = each_chunk_size;
    mempool->total_chunk_size = each_chunk_size;
    mempool->max_mempool_size = max_pool_size;

    void *chunk = (void*) malloc(sizeof(chunk_t) + each_chunk_size);
    if (chunk == NULL){
        fprintf(stderr, "malloc memory for chunk failed. chunk size is %llu\n", each_chunk_size);
        goto err;
    }
    mempool->chunk_list = (chunk_t*) chunk;
    mempool->chunk_list->start = chunk + sizeof(chunk_t);
    init_chunk(chunk, each_chunk_size);
    mempool->chunk_list->id = mempool->last_id++;
    mempool->chunk_list->next = NULL;
    DEBUG("INIT MEMPOOL FINISH");
    return mempool;
    err:
        if (chunk != NULL)
            free(chunk);
        if (mempool != NULL)
            free(mempool);
        return NULL;
}

void* mempool_alloc(mempool_t *mempool, mem_size node_data_size){
    assert(mempool);
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    if (node_data_size <= 0){
        fprintf(stderr, "node data size needed to be allocated must be larger zero");
        goto err;
    }
    mem_size total_need_size = ALIGN_SIZE((NODE_HEADER_SIZE + node_data_size + NODE_TAILER_SIZE));
    if (total_need_size > mempool->each_chunk_size){
        fprintf(stderr, "total need size cannot be larger than each chunk size\n");
        goto err;
    }
    DEBUG("node data size is %llu, total need size is %llu", node_data_size, total_need_size);

    chunk_t *chunk = NULL;
    node_t *free, *not_free;
    FIND_FREE_NODE:
        chunk = mempool->chunk_list;
        while (chunk){
            if (mempool->each_chunk_size - chunk->alloc_chunk_size < total_need_size){
                chunk = chunk->next;
                continue;
            }
            free = chunk->free_list;
            not_free = NULL;
            while (free){
                if (free->node_size >= total_need_size){
                    // split node
                    if (free->node_size > total_need_size + NODE_HEADER_SIZE + NODE_TAILER_SIZE){
                        DEBUG("split node");
                        not_free = free;
                        free = (node_t*) ((void*) not_free + total_need_size);
                        // alloc free node
                        *free = *not_free;
                        free->node_size = free->node_size - total_need_size;
                        *(node_t**) ((void*)free + free->node_size - NODE_TAILER_SIZE) = free;
                        if (free->pre != NULL)
                            free->pre->next = free;
                        else
                            chunk->free_list = free;
                        if (free->next){
                            free->next->pre = free;
                        }
                        // alloc not_free node
                        not_free->is_free = 0;
                        not_free->node_size = total_need_size;
                        *(node_t**)((void*)not_free + total_need_size - NODE_TAILER_SIZE) = not_free;
                    }else {
                        DEBUG("allocate the entire node, total need size is %llu, free node size is %llu", total_need_size, free->node_size);
                        not_free = free;
                        // chunk->free_list = node_delete(chunk->free_list, not_free);
                        node_delete(chunk->free_list, not_free);
                        not_free->is_free = 0;
                    }
                    // chunk->alloc_list = node_insert(chunk->alloc_list, not_free);
                    node_insert(chunk->alloc_list, not_free);
                    chunk->alloc_chunk_size += not_free->node_size;
                    chunk->alloc_data_size += (not_free->node_size - NODE_HEADER_SIZE - NODE_TAILER_SIZE);
                    DEBUG("FIND FREE NODE");
                    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
                    return ((void*)not_free + NODE_HEADER_SIZE);
                }
                free = free->next;
            }
            chunk = chunk->next;
        }
    
        if (mempool->auto_extend){
            if (mempool->total_chunk_size >= mempool->max_mempool_size){
                fprintf(stderr, "mempool total chunk size must be smaller than max mempool size\n");
                goto err;
            }
            mem_size need_add_size = mempool->max_mempool_size - mempool->each_chunk_size;
            need_add_size = need_add_size >= mempool->each_chunk_size ? mempool->each_chunk_size : need_add_size;
            DEBUG("add chunk, the chunk size to be added is %llu", need_add_size);
            chunk_t *add_chunk = extend_chunk_list(mempool, need_add_size);
            if (add_chunk == NULL){
                fprintf(stderr, "malloc memory for the chunk to be added failed, chunk size is %llu\n", need_add_size);
                goto err;
            }
            mempool->total_chunk_size += need_add_size;
            goto FIND_FREE_NODE;
        }
    err:
        if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
        return NULL;
}

void mempool_free(mempool_t *mempool, void *addr){
    assert(mempool);
    assert(addr);
    // find chunk
    DEBUG("free addr is %llu", addr);
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *curr_chunk = mempool->chunk_list;
    while (curr_chunk){
        if (addr >= curr_chunk->start && addr < curr_chunk->start + mempool->each_chunk_size ){
            break;
        }
        curr_chunk = curr_chunk->next;
    }
    // delete current node in alloc list and insert current node in free list
    node_t *curr_node = (node_t*)(addr - NODE_HEADER_SIZE);
    node_delete(curr_chunk->alloc_list, curr_node);
    node_insert(curr_chunk->free_list, curr_node);
    curr_node->is_free = 1;

    curr_chunk->alloc_chunk_size -= curr_node->node_size;
    curr_chunk->alloc_data_size -= (curr_node->node_size - NODE_HEADER_SIZE - NODE_TAILER_SIZE);

    merge_free_node(mempool, curr_chunk, curr_node);
}

mempool_t* mempool_clear(mempool_t *mempool){
    assert(mempool);
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list;
    while (chunk){
        init_chunk(chunk, chunk->mempool_size);
        chunk = chunk->next;
    }
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
    return mempool;
}

int mempool_destroy(mempool_t *mempool){
    assert(mempool);
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list, *chunk_temp;
    while (chunk){
        chunk_temp = chunk;
        chunk = chunk->next;
        free(chunk_temp);
    }
    if (mempool->need_lock) {
        pthread_mutex_unlock(&mempool->lock);
        pthread_mutex_destroy(&mempool->lock);
    }
    free(mempool);
    return 0;
}

static void merge_free_node(mempool_t *mempool, chunk_t *chunk, node_t *node){
    assert(mempool);
    assert(chunk);
    assert(node);
    node_t *next = node, *curr = node;
    while (next->is_free){
        curr = next;
        if ((void*)next - NODE_TAILER_SIZE - NODE_HEADER_SIZE <= chunk->start)
            break;
        next = *(node_t**)((void*)next - NODE_TAILER_SIZE);
    }
    next = (node_t*) ((void*)curr + curr->node_size);
    while ((void*)next < chunk->start + mempool->each_chunk_size && next->is_free){
        // chunk->free_list = node_delete(chunk->free_list, next);
        node_delete(chunk->free_list, next);
        curr->node_size += next->node_size;
        next = (node_t*) ((void*)next + next->node_size);
    }
    *(node_t**)((void*)curr + curr->node_size - NODE_TAILER_SIZE) = curr;
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
}

static chunk_t* extend_chunk_list(mempool_t *mempool, mem_size chunk_size){
    void *mem = (chunk_t*) malloc(sizeof(chunk_t) + chunk_size);
    if (mem == NULL){
        fprintf(stderr, "malloc memory for chunk failed, malloc size is %llu\n", sizeof(chunk_t) + chunk_size);
        return NULL;
    }
    chunk_t *add_chunk = (chunk_t*) mem;
    add_chunk->start = mem + sizeof(chunk_t);
    init_chunk(mem, chunk_size);
    add_chunk->id = mempool->last_id++;
    add_chunk->next = mempool->chunk_list;
    mempool->chunk_list = add_chunk;
    return add_chunk;
}

static void init_chunk(void *chunk_head, mem_size chunk_size){
    chunk_t* chunk = (chunk_t*) chunk_head;
    chunk->mempool_size = chunk_size;
    chunk->alloc_chunk_size = 0;
    chunk->alloc_data_size = 0;
    chunk->alloc_list = NULL;
    chunk->free_list = (node_t*) chunk->start;
    chunk->free_list->is_free = 1;
    chunk->free_list->pre = chunk->free_list->next = NULL;
    chunk->free_list->node_size = chunk_size;
}

mem_size get_total_mempool_size(mempool_t *mempool){
    assert(mempool);
    return mempool->total_chunk_size;
}

mem_size get_used_alloc_chunk_size(mempool_t *mempool){
    assert(mempool);
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list;
    mem_size alloc_chunk_size = 0;
    while (chunk){
        alloc_chunk_size += chunk->alloc_chunk_size;
        chunk = chunk->next;
    }
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
    return alloc_chunk_size;
}

mem_size get_used_alloc_data_size(mempool_t *mempool){
    assert(mempool);
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list;
    mem_size alloc_data_size = 0;
    while (chunk){
        alloc_data_size += chunk->alloc_data_size;
        chunk = chunk->next;
    }
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
    return alloc_data_size;
}

mem_size get_chunk_count(mempool_t *mempool){
    assert(mempool);
    mem_size count = 0;
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list;
    while (chunk){
        count++;
        chunk = chunk->next;
    }
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
    return count;
}

mem_size get_alloc_node_count(mempool_t *mempool){
    assert(mempool);
    mem_size count = 0;
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list;
    node_t *node;
    while (chunk){
        node = chunk->alloc_list;
        while (node){
            count++;
            node = node->next;
        }
        chunk = chunk->next;
    }
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
    return count;
}

mem_size get_free_node_count(mempool_t *mempool){
    assert(mempool);
    mem_size count = 0;
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list;
    node_t *node;
    while (chunk){
        node = chunk->free_list;
        while (node){
            count++;
            node = node->next;
        }
        chunk = chunk->next;
    }
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
    return count;
}

void print_mempool(mempool_t *mempool){
    printf("=======================MemoryPool Info=======================\n");
    printf("total mempool size:%llu, chunk count:%llu, alloc node count:%llu, free node count:%llu\n", get_total_mempool_size(mempool), get_chunk_count(mempool), get_alloc_node_count(mempool), get_free_node_count(mempool));
    printf("the total node size allocated is %llu\n", get_used_alloc_chunk_size(mempool));
    printf("the total data size allocated is %llu\n", get_used_alloc_data_size(mempool));
    if (mempool->need_lock) pthread_mutex_lock(&mempool->lock);
    chunk_t *chunk = mempool->chunk_list;
    while (chunk){
        node_t *alloc_node, *free_node;
        alloc_node = chunk->alloc_list;
        free_node = chunk->free_list;
        mem_size alloc_count = 0, free_count = 0;
        while (alloc_node){
            alloc_count++;
            alloc_node = alloc_node->next;
        }
        while (free_node){
            free_count++;
            free_node = free_node->next;
        }
        printf("chunk id:%d, alloc node count:%llu, free node count:%llu\n", chunk->id, alloc_count, free_count);
        chunk = chunk->next;
    }
    if (mempool->need_lock) pthread_mutex_unlock(&mempool->lock);
}