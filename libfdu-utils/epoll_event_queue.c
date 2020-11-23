#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "epoll_event_queue.h"
#include "utils.h"
#include "nt_log.h"

#ifdef  ENABLE_DEBUG
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);
#else  
DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);
#endif  //ENABLE_DEBUG



static inline bool empty(uint64_t write_index, uint64_t read_index) {
    return write_index == read_index;
}

static inline uint64_t next_index(uint64_t current_idx, uint64_t max_size)
{
    uint64_t ret = current_idx + 1;
    while (UNLIKELY(ret >= max_size))
        ret -= max_size;

    return ret;
}

static inline epoll_event_queue_t _ep_event_queue_init(
            char *shm_addr, size_t addrlen, size_t capacity, bool is_owner)
{
    assert(shm_addr);
    assert(addrlen > 0);
    capacity = LIKELY(capacity > 0) ? capacity : DEFAULT_EP_EVENT_QUEUE_SIZE;

    epoll_event_queue_t queue;
    queue = (epoll_event_queue_t) calloc(1, sizeof(struct _epoll_event_queue));
    if (UNLIKELY(!queue)) {
        ERR("malloc for nts_event_queue_t failed");
        return NULL;
    }
    queue->addrlen = addrlen;
    memcpy(queue->shm_addr, shm_addr, addrlen);
    queue->capacity = capacity;
    queue->queue_bytes = sizeof(nts_event_queue) + capacity * DEFAULT_EP_EVENT_QUEUE_ELE_SIZE;

    sprintf(queue->mutex_sem_name, "%s%s", EP_SEM_MUTEX_PREFIX, shm_addr);
    sprintf(queue->full_sem_name, "%s%s", EP_SEM_FULL_PREFIX, shm_addr);
    sprintf(queue->empty_sem_name, "%s%s", EP_SEM_EMPTY_PREFIX, shm_addr);

    // get shared memory for shm-based event_queue
    int shm_mask = (is_owner ? O_RDWR | O_CREAT : O_RDWR);
    mode_t shm_mode = is_owner ? 0666 : 0;
    queue->shm_fd = shm_open(queue->shm_addr, shm_mask, shm_mode);
    if (UNLIKELY(queue->shm_fd == -1)) {
        if (is_owner && (errno == ENOENT || errno == EEXIST)) {
            queue->shm_fd = shm_open(queue->shm_addr, shm_mask, shm_mode);
            if (queue->shm_fd == -1) {
                perror("shm_open");
                ERR("shm_open '%s' failed!", queue->shm_addr);
                goto FAIL;
            }
        } else {
            perror("shm_open");
            ERR("shm_open '%s' failed!", queue->shm_addr);
            goto FAIL;
        }
    }

    // set the permission of shm fd for write/read in non-root users 
    fchmod(queue->shm_fd, 0666);
    DEBUG("shm_open pass"); 

    int ret;
    if (is_owner) {
        ret = ftruncate(queue->shm_fd, queue->queue_bytes);
        if (ret == -1) {
            perror("ftruncate");
            goto FAIL;
        }
        DEBUG("ftruncate pass");
    }

    // mmap the allocated shared memory to epoll_event_queue
    queue->shm_queue = (nts_event_queue_t) mmap(
                    NULL, queue->queue_bytes, 
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    queue->shm_fd, 0);
    if (queue->shm_queue == MAP_FAILED) {
        perror("mmap epoll_event_queue");
        ERR("mmap epoll_event_queue failed");
        goto FAIL;
    }
    if (is_owner) {
        queue->shm_queue->start = queue->shm_queue->end = 0;
        queue->shm_queue->num_events = 0;
    }
    queue->shm_queue->size = capacity;
    queue->events = (nts_epoll_event_int *) ((uint8_t *)queue->shm_queue + sizeof(nts_event_queue));

    // store old, umask for world-writable access of semaphores (mutex_sem, buf_count_sem, spool_signal_sem)
    mode_t old_umask; 
    old_umask = umask(0);

    // mutual exclusion semaphore, mutex_sem with an initial value 0.
    queue->mutex_sem = sem_open(queue->mutex_sem_name, O_CREAT | O_RDWR, 0666, 0);
    if (UNLIKELY(queue->mutex_sem == SEM_FAILED)) {
        perror("sem_open: mutex_sem");
        ERR("sem_open mutex_sem for epoll_event_queue failed");
        // restore old mask
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open mutex_sem pass");

    // counting semaphore, indicating the number of available buffers in shm ring.
    // init value = NTM_MAX_BUFS
    queue->empty_sem = sem_open(queue->empty_sem_name, O_CREAT | O_RDWR, 0666, queue->capacity);
    if (UNLIKELY(queue->empty_sem == SEM_FAILED)) {
        perror("sem_open: empty_sem");
        ERR("sem_open empty_sem for epoll_event_queue failed");
        // restore old mask
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open empty_sem pass");

    // count semaphore, indicating the number of data buffers to be read. Init value = 0
    queue->full_sem = sem_open(queue->full_sem_name, O_CREAT | O_RDWR, 0666, 0);
    if (UNLIKELY(queue->full_sem == SEM_FAILED)) {
        perror("sem_open: full_sem");
        ERR("sem_open full_sem for epoll_event_queue failed");
        // restore old mask
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open full_sem pass");

    // restore old mask
    umask(old_umask);

    if (is_owner) {
        // init complete; now set mutex semaphore as 1 to
        // indicate the shared memory segment is available
        ret = sem_post(queue->mutex_sem);
        if (UNLIKELY(ret == -1)) {
            perror("sem_post: mutex_sem");
            ERR("sem_post mutex_sem for sem_shmring failed");
            goto FAIL;
        }
    }

    return queue;

FAIL:
    if (queue->full_sem != SEM_FAILED) {
        sem_close(queue->full_sem);
        if (is_owner) {
            sem_unlink(queue->full_sem_name);
        }
    }
    if (queue->empty_sem != SEM_FAILED) {
        sem_close(queue->empty_sem);
        if (is_owner) { 
            sem_unlink(queue->empty_sem_name);
        }
    }
    if (queue->mutex_sem != SEM_FAILED) {
        sem_close(queue->mutex_sem);
        if (is_owner) { 
            sem_unlink(queue->mutex_sem_name);
        }
    }

    if (queue->shm_queue) {
        munmap((void *)queue->shm_queue, queue->queue_bytes);
    }
    if (queue->shm_fd) {
        close(queue->shm_fd);
        if (is_owner) { 
            shm_unlink(queue->shm_addr);
        }
    }
  
    free(queue);
    return NULL;
}

// When master first create shm-based event_queue
epoll_event_queue_t ep_event_queue_create(char *shm_addr, size_t addrlen, size_t capacity)
{

    return _ep_event_queue_init(shm_addr, addrlen, capacity, true);
}

// When slave first try to init/connect existing shm-based event_queue
epoll_event_queue_t ep_event_queue_init(char *shm_addr, size_t addrlen, size_t capacity)
{

    return _ep_event_queue_init(shm_addr, addrlen, capacity, false);
}

// Returns 0 on success, -1 if buffer is full
int ep_event_queue_push(epoll_event_queue_t ctx, nts_epoll_event_int *event)
{
    assert(ctx);
    assert(event);


    int retval;
    // get a buffer: P (empty_sem)
    retval = sem_wait(ctx->empty_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[ep_event_queue_push()] sem_wait: empty_sem");
        goto FAIL;
    }

    /**
     * There might be multiple producers. We must ensure that
     * only one producer uses write_index at a time.
     */
    retval = sem_wait(ctx->mutex_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[ep_event_queue_push()] sem_wait: mutex_sem");
        goto FAIL;
    }
	DEBUG("enter critical section...");

    /* Critical Section */
    uint64_t write_idx;
    write_idx = ctx->shm_queue->end;
    memcpy((void *)&ctx->events[write_idx], (void *)event, DEFAULT_EP_EVENT_QUEUE_ELE_SIZE);
    __WRITE_BARRIER__;
    ctx->shm_queue->end = next_index(ctx->shm_queue->end, ctx->shm_queue->size);
    ctx->shm_queue->num_events ++;

    // Release mutex sem: V (mutex_sem)
    retval = sem_post(ctx->mutex_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[ep_event_queue_push()] sem_post: mutex_sem");
        goto FAIL;
    }

    // tell spooler that there is a element to read: V (full_sem)
    retval = sem_post(ctx->full_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[ep_event_queue_push()] sem_post: full_sem");
        goto FAIL;
    }
    DEBUG("push event queue success");
    return 0;

FAIL:
    return -1;
}

//  Returns 0 on success, -1 if the buffer is empty
int ep_event_queue_pop(epoll_event_queue_t ctx, nts_epoll_event_int *event)
{
    assert(ctx);
    assert(event);

    int retval;
    // Is there a shm element to read ? P(full_sem)
    retval = sem_wait(ctx->full_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[ep_event_queue_pop()] sem_wait: spool_signal_sem");
        return -1;
    }

    // Considering libnts is the only event consumer and 
    // there is no resource contention, 
    // we don't use 'mutex_sem' to sync state
    uint64_t read_idx;
    read_idx = ctx->shm_queue->start;
    memcpy((void *)event, (void *)&ctx->events[read_idx], DEFAULT_EP_EVENT_QUEUE_ELE_SIZE);
    __WRITE_BARRIER__;
    ctx->shm_queue->start = next_index(ctx->shm_queue->start, ctx->shm_queue->size);
    ctx->shm_queue->num_events --;

    // tell spooler that there is a element to read: V (empty_sem)
    retval = sem_post(ctx->empty_sem);
    if (retval == -1) {
        ERR("[ep_event_queue_pop()] sem_post: empty_sem");
        return -1;
    }
    DEBUG("pop event queue success");
    return 0;
}

//  Returns 0 on success, -1 if the buffer is empty
int ep_event_queue_front(epoll_event_queue_t ctx, nts_epoll_event_int *event)
{
    assert(ctx);
    assert(event);
   
    if (empty(ctx->shm_queue->end, ctx->shm_queue->start)) {
        ERR("event queue is empty");
        return -1;
    }
    uint64_t read_idx;
    read_idx = ctx->shm_queue->start;
    memcpy((void *)event, (void *)&ctx->events[read_idx], DEFAULT_EP_EVENT_QUEUE_ELE_SIZE);
    __WRITE_BARRIER__;

    DEBUG("front event queue success");
    return 0;
}

// Free epoll_event_queue structure, if unlink is 1, unlink all SHM-based variables.
int ep_event_queue_free(epoll_event_queue_t ctx, bool is_unlink)
{
    assert(ctx);

    if (ctx->full_sem != SEM_FAILED) {
        sem_close(ctx->full_sem);
        if (is_unlink) {
            sem_unlink(ctx->full_sem_name);
        }
    }
    if (ctx->empty_sem != SEM_FAILED) {
        sem_close(ctx->empty_sem);
        if (is_unlink) { 
            sem_unlink(ctx->empty_sem_name);
        }
    }
    if (ctx->mutex_sem != SEM_FAILED) {
        sem_close(ctx->mutex_sem);
        if (is_unlink) { 
            sem_unlink(ctx->mutex_sem_name);
        }
    }

    if (ctx->shm_queue) {
        munmap((void *)ctx->shm_queue, ctx->queue_bytes);
    }
    if (ctx->shm_fd) {
        close(ctx->shm_fd);
        if (is_unlink) { 
            shm_unlink(ctx->shm_addr);
        }
    }
    free(ctx);
    return 0;
}