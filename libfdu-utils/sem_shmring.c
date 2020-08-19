#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#include "sem_shmring.h"
#include "utils.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define SEM_SHM_MAX_ADDRLEN 64

typedef struct sem_shmring_buf {
    uint64_t write_idx; // tail pointer
    uint64_t read_idx;  // head pointer
    uint8_t is_full;    // 1 indicates full while 0 indicates not full
    uint8_t * data_area;   // for store data
} sem_shmring_buf;

typedef sem_shmring_buf *circular_queue_t;

typedef struct _sem_shmring {
    uint64_t shm_fd;
    uint64_t addrlen;
    uint64_t ele_size;      // the size of each element which is specified during initing
    uint64_t capacity;       // the number of total circular buffer
    uint64_t queue_size;    // = sizeof(struct sem_shmring_buf) + ele_size * capacity

    char mutex_sem_name[SEM_SHM_MAX_ADDRLEN];
    char buf_count_sem_name[SEM_SHM_MAX_ADDRLEN];
    char spool_signal_sem_name[SEM_SHM_MAX_ADDRLEN];
    char shm_addr[SEM_SHM_MAX_ADDRLEN];

    sem_t * mutex_sem;
    sem_t * buf_count_sem;
    sem_t * spool_signal_sem;

    circular_queue_t queue;
    char **data;            // organize the memory addr of each queue element, 
                            // like data[i] = queue->data_area + i * ele_size
} _sem_shmring;


static inline uint64_t next_index(uint64_t current_idx, uint64_t max_size) {
    uint64_t ret = current_idx + 1;
    while (UNLIKELY(ret >= max_size))
        ret -= max_size;

    return ret;
}

static inline bool empty(uint64_t write_index, uint64_t read_index) {
    return write_index == read_index;
}

/**
 * @brief  
 * @note   
 * @param  *shm_addr: 
 * @param  addrlen: 
 * @param  ele_size: 
 * @param  ele_num: 
 * @param  is_owner: indicate whether the invoker is owner of the sem_shmring, 
 *                  if true, means sem_shmring_create(); 
 *                  otherwise, means sem_shmring_init()
 * @retval if success, return sem_shmring_handle_t; otherwise, failed
 */
static inline sem_shmring_handle_t _sem_shmring_init(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num, bool is_owner) {
    assert(shm_addr);
    assert(addrlen > 0);
    assert(ele_size > 0);
    ele_num = LIKELY(ele_num > 0) ? ele_num : DEFAULT_SHMRING_MAX_BUFS;

    sem_shmring_handle_t handle;
    handle = (sem_shmring_handle_t) calloc(1, sizeof(sem_shmring_t));
    if (!handle) {
        ERR("malloc sem_shmring_handle_t failed");
        return NULL;
    }
    handle->addrlen = addrlen;
    memcpy(handle->shm_addr, shm_addr, addrlen);
    handle->ele_size = ele_size;
    handle->capacity = ele_num;
    handle->queue_size = sizeof(struct sem_shmring_buf) + ele_size * ele_num;

    sprintf(handle->mutex_sem_name, "%s%s", SEM_MUTEX_PREFIX, shm_addr);
    sprintf(handle->buf_count_sem_name, "%s%s", SEM_BUF_COUNT_PREFIX, shm_addr);
    sprintf(handle->spool_signal_sem_name, "%s%s", SEM_SPOOL_SIGNAL_PREFIX, shm_addr);

    handle->data = (char **) calloc(handle->capacity, sizeof(char *));
    if (!handle->data) {
        ERR("malloc handle->data (char **) failed");
        free(handle);
        return NULL;
    }

    // get shared memory for shmring
    int shm_mask = (is_owner ? O_RDWR | O_CREAT : O_RDWR);
    mode_t shm_mode = is_owner ? 0666 : 0;
    handle->shm_fd = shm_open(handle->shm_addr, shm_mask, shm_mode);
    if (UNLIKELY(handle->shm_fd == -1)) {
        if (is_owner && (errno == ENOENT || errno == EEXIST)) {
            handle->shm_fd = shm_open(handle->shm_addr, shm_mask, shm_mode);
            if (handle->shm_fd == -1) {
                perror("shm_open");
                ERR("shm_open '%s' failed!", handle->shm_addr);
                goto FAIL;
            }
        } else {
            perror("shm_open");
            ERR("shm_open '%s' failed!", handle->shm_addr);
            goto FAIL;
        }
    }

    // set the permission of shm fd for write/read in non-root users 
    fchmod(handle->shm_fd, 0666);
    DEBUG("shm_open pass");

    int ret;
    if (is_owner) {
        ret = ftruncate(handle->shm_fd, handle->queue_size);
        if (ret == -1) {
            perror("ftruncate");
            goto FAIL;
        }
        DEBUG("ftruncate pass");
    }
    
    // mmap the allocated shared memory to shmring
    handle->queue = (circular_queue_t)
            mmap(NULL, handle->queue_size,
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 handle->shm_fd, 0);
    if (handle->queue == MAP_FAILED) {
        perror("mmap sem_shmring");
        ERR("mmap sem_shmring failed");
        goto FAIL;
    }
    if (is_owner) {
        handle->queue->write_idx = handle->queue->read_idx = 0;
        handle->queue->is_full = 0;
    }
    handle->queue->data_area = (uint8_t *) ((uint8_t *)handle->queue + sizeof(struct sem_shmring_buf));
    for(int i = 0; i < handle->capacity; i++) {
        handle->data[i] = (char *) (handle->queue->data_area + i * handle->ele_size);
    }

    // store old, umask for world-writable access of semaphores (mutex_sem, buf_count_sem, spool_signal_sem)
    mode_t old_umask; 
    old_umask = umask(0);

    // mutual exclusion semaphore, mutex_sem with an initial value 0.
    handle->mutex_sem = sem_open(handle->mutex_sem_name, O_CREAT, 0666, 0);
    if (handle->mutex_sem == SEM_FAILED) {
        perror("sem_open: mutex_sem");
        ERR("sem_open mutex_sem for sem_shmring failed");
        // restore old mask
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open mutex_sem pass");

    // counting semaphore, indicating the number of available buffers in shm ring.
    // init value = NTM_MAX_BUFS
    handle->buf_count_sem = sem_open(handle->buf_count_sem_name, O_CREAT, 0666, handle->capacity);
    if ((sem_t *)handle->buf_count_sem == SEM_FAILED) {
        perror("sem_open: buf_count_sem");
        ERR("sem_open buf_count_sem for sem_shmring failed");
        // restore old mask
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open buf_count_sem pass");

    // count semaphore, indicating the number of data buffers to be read. Init value = 0
    handle->spool_signal_sem = sem_open(handle->spool_signal_sem_name, O_CREAT, 0666, 0);
    if (handle->spool_signal_sem == SEM_FAILED) {
        perror("sem_open: spool_signal_sem");
        ERR("sem_open spool_signal_sem for sem_shmring failed");
        // restore old mask
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open spool_signal_sem pass");

    // restore old mask
    umask(old_umask);

    if (is_owner) {
        // init complete; now set mutex semaphore as 1 to
        // indicate the shared memory segment is available
        ret = sem_post(handle->mutex_sem);
        if (ret == -1) {
            perror("sem_post: mutex_sem");
            ERR("sem_post mutex_sem for sem_shmring failed");
            goto FAIL;
        }
    }
    
    return handle;

FAIL:
    if (handle->spool_signal_sem != SEM_FAILED) {
        sem_close(handle->spool_signal_sem);
        if (is_owner) {
            sem_unlink(handle->spool_signal_sem_name);
        }
    }
    if (handle->buf_count_sem != SEM_FAILED) {
        sem_close(handle->buf_count_sem);
        if (is_owner) { 
            sem_unlink(handle->buf_count_sem_name);
        }
    }
    if (handle->mutex_sem != SEM_FAILED) {
        sem_close(handle->mutex_sem);
        if (is_owner) { 
            sem_unlink(handle->mutex_sem_name);
        }
    }

    if (handle->queue) {
        munmap((void *)handle->queue, handle->queue_size);
    }
    if (handle->shm_fd) {
        close(handle->shm_fd);
        if (is_owner) { 
            shm_unlink(handle->shm_addr);
        }
    }
    if (handle->data) {
        free((void *)handle->data);
    }
    free(handle);
    return NULL;
}

// When master first create sem_shmring
sem_shmring_handle_t sem_shmring_create(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num) {
    return _sem_shmring_init(shm_addr, addrlen, ele_size, ele_num, true);
}

// When slave first try to init/connect existing sem_shmring
sem_shmring_handle_t sem_shmring_init(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num) {
    return _sem_shmring_init(shm_addr, addrlen, ele_size, ele_num, false);
}

// Returns 0 on success, -1 if buffer is full
int sem_shmring_push(sem_shmring_handle_t handle, char *element, size_t ele_len) {
    assert(handle);
    assert(element);
    int retval;
    ele_len = ele_len > handle->ele_size ? handle->ele_size : ele_len;

    // get a buffer: P (buf_count_sem)
    retval = sem_wait(handle->buf_count_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[sem_shmring_push()] sem_wait: buf_count_sem");
        goto FAIL;
    }

    /**
     * There might be multiple producers. We must ensure that
     * only one producer uses write_index at a time.
     */
    retval = sem_wait(handle->mutex_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[sem_shmring_push()] sem_wait: mutex_sem");
        goto FAIL;
    }
	DEBUG("enter critical section... ");

    /* Critical Section */
    memcpy(handle->data[handle->queue->write_idx], element, ele_len);
    handle->queue->write_idx = next_index(handle->queue->write_idx, handle->capacity);
    if (handle->queue->write_idx == handle->queue->read_idx) {
        handle->queue->is_full = 1;
    }

    // Release mutex sem: V (mutex_sem)
    retval = sem_post(handle->mutex_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[sem_shmring_push()] sem_post: mutex_sem");
        goto FAIL;
    }

    // tell spooler that there is a element to read: V (spool_signal_sem)
    retval = sem_post(handle->spool_signal_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[sem_shmring_push()] sem_post: spool_signal_sem");
        goto FAIL;
    }
	DEBUG("sem_shmring_push success!");
    return 0;

FAIL: 
    return -1;
}

//  Returns 0 on success, -1 if the buffer is empty
int sem_shmring_pop(sem_shmring_handle_t handle, char *element, size_t ele_len) {
    assert(handle);
    assert(element);
    int retval;
    ele_len = ele_len > handle->ele_size ? handle->ele_size : ele_len;

    // Is there a shm element to read ? P(spool_signal_sem)
    retval = sem_wait(handle->spool_signal_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[sem_shmring_pop()] sem_wait: spool_signal_sem");
        goto FAIL;
    }

    DEBUG("write_idx=%ld, read_idx=%ld", handle->queue->write_idx, handle->queue->read_idx);
    memcpy(element, handle->data[handle->queue->read_idx], ele_len);

    handle->queue->is_full = 0;
    handle->queue->read_idx = next_index(handle->queue->read_idx, handle->capacity);

     /**
     * one element of shm ring has been pop.
     * One more buffer is available for push by producers (libnts app).
     * Release buffer: V (buf_count_sem);
     */
    retval = sem_post(handle->buf_count_sem);
    if (UNLIKELY(retval == -1)) {
        ERR("[sem_shmring_pop()] sem_post: buf_count_sem");
        goto FAIL;
    }
    DEBUG("sem_shmring_pop success");
    return 0;
FAIL: 
    return -1;
}

//  Returns 0 on success, -1 if the buffer is empty
int sem_shmring_front(sem_shmring_handle_t handle, char *element, size_t ele_len) {
    assert(handle);
    assert(element);
    ele_len = ele_len > handle->ele_size ? handle->ele_size : ele_len;

    if (empty(handle->queue->write_idx, handle->queue->read_idx)) {
        ERR("sem_shmring is empty");
        return -1;
    }
    memcpy(element, handle->data[handle->queue->read_idx], ele_len);

    DEBUG("sem_shmring_pop success");
    return 0;
}

// Free sem_shmring structure, if unlink is 1, unlink all SHM-based variables.
int sem_shmring_free(sem_shmring_handle_t handle, bool is_unlink) {
    assert(handle);

    if (handle->spool_signal_sem != SEM_FAILED) {
        sem_close(handle->spool_signal_sem);
        if (is_unlink) {
            sem_unlink(handle->spool_signal_sem_name);
        }
    }
    if (handle->buf_count_sem != SEM_FAILED) {
        sem_close(handle->buf_count_sem);
        if (is_unlink) { 
            sem_unlink(handle->buf_count_sem_name);
        }
    }
    if (handle->mutex_sem != SEM_FAILED) {
        sem_close(handle->mutex_sem);
        if (is_unlink) { 
            sem_unlink(handle->mutex_sem_name);
        }
    }

    if (handle->queue) {
        munmap((void *)handle->queue, handle->queue_size);
    }
    if (handle->shm_fd) {
        close(handle->shm_fd);
        if (is_unlink) { 
            shm_unlink(handle->shm_addr);
        }
    }
    if (handle->data) {
        free((void *)handle->data);
    }
    free(handle);
    return 0;
}

// Reset the sem_shmring circular buffer to empty, head == tail
int sem_shmring_reset(sem_shmring_handle_t handle) {
    assert(handle);

    handle->queue->write_idx = 0;
    handle->queue->read_idx = 0;
    handle->queue->is_full = 0;
    return 0;
}

// Returns true if the buffer is empty
bool sem_shmring_empty(sem_shmring_handle_t handle) {
    assert(handle);
    return (!handle->queue->is_full && empty(handle->queue->write_idx, handle->queue->read_idx));
}

// Returns true if the buffer is full
bool sem_shmring_full(sem_shmring_handle_t handle) {
    assert(handle);
    return (handle->queue->is_full == 1 ? true : false);
}

// Returns the maximum capacity of the buffer
size_t sem_shmring_capacity(sem_shmring_handle_t handle) {
    assert(handle);
    return handle->capacity;
}

// Returns the current number of elements in the buffer
size_t sem_shmring_size(sem_shmring_handle_t handle) {
    assert(handle);

    size_t size = handle->capacity;
    if (!handle->queue->is_full) {
        if (handle->queue->write_idx >= handle->queue->read_idx) {
            size = handle->queue->write_idx - handle->queue->read_idx;
        } else {
            size = handle->capacity + handle->queue->write_idx - handle->queue->read_idx;
        }
    }
    return size;
}

