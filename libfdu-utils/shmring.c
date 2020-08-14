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
#include <sys/time.h>

#include "shmring.h"
#include "nt_atomic.h"
#include "utils.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);

#define SHM_MAX_ADDRLEN 64

typedef struct shmring_buf {
    uint64_t write_idx;
    uint64_t read_idx;
    uint8_t * data_area;    // for store data
} shmring_buf;
typedef shmring_buf *ring_queue_t;

typedef struct _shmring {
    uint64_t shm_fd;
    uint64_t addrlen;
    uint64_t ele_size;      // the size of each element which is specified during initing
    uint64_t capacity;       // the number of total circular buffer
    uint64_t queue_size;    // = sizeof(struct shmring_buf) + ele_size * capacity
    uint64_t peer_read_idx;   //read_idx of opposite recv buffer
    char shm_addr[SHM_MAX_ADDRLEN];
    ring_queue_t queue;
    char **data;            // organize the memory addr of each queue element, 
                            // like data[i] = queue->data_area + i * ele_size
} _shmring;


static inline uint64_t increment(uint64_t current_idx, uint64_t max_size) {

    return (current_idx + 1) % max_size;
}

static inline uint64_t mask_increment(uint64_t current_idx, uint64_t mask) {

    return (current_idx + 1) & mask;
}

static inline uint64_t next_index(uint64_t current_idx, uint64_t max_size) {
    uint64_t ret = current_idx + 1;
    while (UNLIKELY(ret >= max_size))
        ret -= max_size;

    return ret;
}

static inline bool empty(uint64_t write_idx, uint64_t read_idx) {
    return write_idx == read_idx;
}


shmring_handle_t _shmring_init(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num, bool is_owner) {
    assert(shm_addr);
    assert(addrlen > 0);
    assert(ele_size > 0);
    ele_num = (ele_num > 0) ? ele_num : DEFAULT_MAX_BUFS;

    shmring_handle_t handle;
    handle = (shmring_handle_t) calloc(1, sizeof(shmring_t));
    if (!handle) {
        ERR("malloc shmring_handle_t failed");
        return NULL;
    }
    handle->addrlen = addrlen;
    memcpy(handle->shm_addr, shm_addr, addrlen);
    handle->ele_size = ele_size;
    handle->capacity = ele_num;
    handle->queue_size = sizeof(struct shmring_buf) + ele_size * ele_num;
    handle->data = (char **) calloc(handle->capacity, sizeof(char *));
    if (!handle->data) {
        ERR("malloc handle->data (char **) failed");
        free(handle);
        return NULL;
    }

    int shm_mask = (is_owner ? O_RDWR | O_CREAT : O_RDWR);
    mode_t shm_mode = is_owner ? 0666 : 0;
    handle->shm_fd = shm_open(handle->shm_addr, shm_mask, shm_mode);
    if (handle->shm_fd == -1) {
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
    handle->queue = (ring_queue_t)
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
    }
    handle->peer_read_idx = 0;
    handle->queue->data_area = (uint8_t *) ((uint8_t *)handle->queue + sizeof(struct shmring_buf));
    for(int i = 0; i < handle->capacity; i++) {
        handle->data[i] = (char *) (handle->queue->data_area + i * handle->ele_size);
    }
    return handle;

FAIL: 
    if (handle->queue) {
        munmap((void*) handle->queue, handle->queue_size);
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

shmring_handle_t shmring_create(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num) { 

    return _shmring_init(shm_addr, addrlen, ele_size, ele_num, true);
}


shmring_handle_t shmring_init(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num) { 

    return _shmring_init(shm_addr, addrlen, ele_size, ele_num, false);
}


bool shmring_push(shmring_handle_t self, char *element, size_t ele_len) {
    assert(self);
    assert(element);

    ele_len = UNLIKELY(ele_len > self->ele_size) ? self->ele_size : ele_len;

    /* Critical Section */
//    const uint64_t w_idx = nt_atomic_load64_explicit(
//            &self->queue->write_idx, ATOMIC_MEMORY_ORDER_CONSUME);

    const uint64_t w_idx = nt_atomic_load64_explicit(
            &self->queue->write_idx, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Assuming we write, where will move next ?
    const uint64_t w_next_idx = next_index(w_idx, self->capacity);

    /// The two pointers colliding means we would have exceeded the
    /// ring buffer size and create an ambiguous state with being empty.
    if (UNLIKELY(w_next_idx == nt_atomic_load64_explicit(
            &self->queue->read_idx, ATOMIC_MEMORY_ORDER_ACQUIRE))){
        INFO("shmring is full");
        return false;
    }

    memset(self->data[w_idx], 0, self->ele_size);
    memcpy(self->data[w_idx], element, ele_len);
    DEBUG("[after push] value=%s", self->data[w_idx]);

    nt_atomic_store64_explicit(&self->queue->write_idx,
                               w_next_idx, ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("push nts shmring successfully!");
    return true;
}

bool shmring_pop(shmring_handle_t self, char *element, size_t ele_len) {
    assert(self);
    assert(element);
    DEBUG("ntp_shmring_pop start with shmaddr='%s'.", self->shm_addr);

    ele_len = UNLIKELY(ele_len > self->ele_size) ? self->ele_size : ele_len;

    uint64_t w_idx = nt_atomic_load64_explicit(&self->queue->write_idx, ATOMIC_MEMORY_ORDER_ACQUIRE);
    uint64_t r_idx = nt_atomic_load64_explicit(&self->queue->read_idx, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Queue is empty (or was empty when we checked)
    if (empty(w_idx, r_idx))
        return false;

    DEBUG("ntp_msgcopy start with write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
    memset(element, 0, ele_len);
    memcpy(element, self->data[self->queue->read_idx], ele_len);
    DEBUG("[pop]value=%s", self->data[self->queue->read_idx]);

    nt_atomic_store64_explicit(&self->queue->read_idx,
                               next_index(r_idx, self->capacity), ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("pop shmring successfully!");
    return true;
}

bool shmring_front(shmring_handle_t self, char *element, size_t ele_len) {
    assert(self);
    assert(element);

    ele_len = UNLIKELY(ele_len > self->ele_size) ? self->ele_size : ele_len;

    uint64_t w_idx = nt_atomic_load64_explicit(&self->queue->write_idx, ATOMIC_MEMORY_ORDER_ACQUIRE);
    uint64_t r_idx = nt_atomic_load64_explicit(&self->queue->read_idx, ATOMIC_MEMORY_ORDER_RELAXED);
    
    DEBUG("shmring_front write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
   /// Queue is empty (or was empty when we checked)
   if (empty(w_idx, r_idx)){
       return false;
   }
       
    DEBUG("ntp_msgcopy start with write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
    memset(element, 0, ele_len);
    memcpy(element, self->data[self->queue->read_idx], ele_len);
    DEBUG("[front]value=%s", self->data[self->queue->read_idx]);


    DEBUG("front shmring successfully!");

    return true;
}

void shmring_free(shmring_handle_t handle, bool is_unlink) {
    assert(handle);
    DEBUG("nts shmring free start");

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
    DEBUG("free shmring successfully!");
}


uint64_t ntp_get_read_idx(shmring_handle_t self)
{
    const uint64_t r_idx = nt_atomic_load64_explicit(
        &self->queue->read_idx, ATOMIC_MEMORY_ORDER_RELAXED);
    return r_idx;
}

uint64_t ntp_get_peer_read_idx(shmring_handle_t self)
{
    const uint64_t op_idx = nt_atomic_load64_explicit(
        &self->peer_read_idx, ATOMIC_MEMORY_ORDER_RELAXED);
    return op_idx;
}

int ntp_set_peer_read_idx(shmring_handle_t self, uint64_t read_idx)
{
    nt_atomic_store64_explicit(&self->peer_read_idx,
                               read_idx, ATOMIC_MEMORY_ORDER_RELAXED);
    return 0;
}
