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
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);

typedef struct shmring_buf {
//    char buf[NTS_MAX_BUFS + 1][NTS_BUF_SIZE];
	// nts_msg buf[NTS_MAX_BUFS + 1];
    char buf[MAX_BUFS + 1][BUF_SIZE];
    uint64_t write_index;
    uint64_t read_index;
    
} shmring_buf;

typedef struct _shmring {
    int shm_fd;
    unsigned long MASK;
    int addrlen;
    char *shm_addr;
    //read_index of opposite recv buffer
    uint64_t peer_read_index;
    struct shmring_buf *shmring;
    uint64_t max_size;
} _shmring;


static inline uint64_t increment(uint64_t current_idx, uint64_t max_size) {

    return (current_idx + 1) % max_size;
}

static inline uint64_t mask_increment(uint64_t current_idx, uint64_t mask) {

    return (current_idx + 1) & mask;
}

static inline uint64_t next_index(uint64_t current_idx, uint64_t max_size) {
    uint64_t ret = current_idx + 1;
    while (NTS_UNLIKELY(ret >= max_size))
        ret -= max_size;

    return ret;
}

static inline bool empty(uint64_t write_index, uint64_t read_index) {
    return write_index == read_index;
}


shmring_handle_t shmring_init(char *shm_addr, size_t addrlen) {
    assert(shm_addr);
	assert(addrlen > 0);

    shmring_handle_t shmring_handle;

    shmring_handle = (shmring_handle_t) malloc(sizeof(shmring_t));
    DEBUG("init shmring");
    shmring_handle->addrlen = addrlen;
    shmring_handle->shm_addr = shm_addr;

    // get shared memory for shmring
    shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR | O_CREAT, 0666);
    if (shmring_handle->shm_fd == -1) {
        if (errno == ENOENT || errno == EEXIST) {
            shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR | O_CREAT, 0666);
            if (shmring_handle->shm_fd == -1) {
                perror("shm_open");
                goto FAIL;
            }
        } else {
            perror("shm_open");
            goto FAIL;
        }
    }
    // set the permission of shm fd for write/read in non-root users 
    fchmod(shmring_handle->shm_fd, 0666);
    DEBUG("shm_open pass");

    int ret;
    ret = ftruncate(shmring_handle->shm_fd, sizeof(struct shmring_buf));
    if (ret == -1) {
        perror("ftruncate");
        goto FAIL;
    }
    DEBUG("ftruncate pass");

    // mmap the allocated shared memory to shmring
    shmring_handle->shmring = (struct shmring_buf *)
            mmap(NULL, sizeof(struct shmring_buf),
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shmring_handle->shm_fd, 0);
    if (shmring_handle->shmring == MAP_FAILED) {
        perror("mmap");
        goto FAIL;
    }
    // init the shared memory
    shmring_handle->shmring->read_index = shmring_handle->shmring->write_index = 0;
    shmring_handle->peer_read_index = 0;
    DEBUG("mmap pass");

    shmring_handle->MASK = MAX_BUFS - 1;
    shmring_handle->max_size = MAX_BUFS;
    DEBUG("nts shmring init successfully!");


    return shmring_handle;

    FAIL:
    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);
        shm_unlink(shmring_handle->shm_addr);
    }

    free(shmring_handle);

    return NULL;
}

shmring_handle_t get_shmring(char *shm_addr, size_t addrlen) {
    assert(shm_addr);
	assert(addrlen > 0);

    shmring_handle_t shmring_handle;
    DEBUG("nts get shmring start");

    shmring_handle = (shmring_handle_t) malloc(sizeof(shmring_t));
    memset(shmring_handle, 0, sizeof(shmring_t));
    shmring_handle->addrlen = addrlen;
    shmring_handle->shm_addr = shm_addr;
    DEBUG("ntp shm_addr='%s', addrlen=%d", shm_addr, (int)addrlen);

    // get shared memory with specified SHM NAME
    shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR, 0);
    if (shmring_handle->shm_fd == -1) {
        perror("shm_open");
        goto FAIL;
    }
    // set the permission of shm fd for write/read in non-root users 
    fchmod(shmring_handle->shm_fd, 0666);
    DEBUG("shm_open pass with fd - %d", shmring_handle->shm_fd);

    // mmap the allocated shared memory to shmring
    shmring_handle->shmring = (struct shmring_buf *)
            mmap(NULL, sizeof(struct shmring_buf),
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shmring_handle->shm_fd, 0);
    if (shmring_handle->shmring == MAP_FAILED) {
        perror("mmap");
        goto FAIL;
    }
    DEBUG("mmap pass");
    
    shmring_handle->MASK = MAX_BUFS - 1;
    shmring_handle->max_size = MAX_BUFS;
    shmring_handle->peer_read_index = 0;
    DEBUG("nts get shmring successfully!");

    return shmring_handle;

    FAIL:
    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);
    }

    free(shmring_handle);

    return NULL;
}

bool shmring_push(shmring_handle_t self, char *element, size_t ele_len) {
    assert(self);
    assert(element);
    // DEBUG("ntp_shmring_push start with shmaddr='%s'.", self->shm_addr);

    ele_len = ele_len > BUF_SIZE ? BUF_SIZE : ele_len;

    /* Critical Section */
    const uint64_t r_idx = nt_atomic_load64_explicit(
            &self->shmring->read_index, ATOMIC_MEMORY_ORDER_CONSUME);
//    const uint64_t w_idx = nt_atomic_load64_explicit(
//            &self->shmring->write_index, ATOMIC_MEMORY_ORDER_CONSUME);

    const uint64_t w_idx = nt_atomic_load64_explicit(
            &self->shmring->write_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Assuming we write, where will move next ?
    const uint64_t w_next_idx = mask_increment(w_idx, self->MASK);

    /// The two pointers colliding means we would have exceeded the
    /// ring buffer size and create an ambiguous state with being empty.
    if (w_next_idx == nt_atomic_load64_explicit(
            &self->shmring->read_index, ATOMIC_MEMORY_ORDER_ACQUIRE)){
        DEBUG("shmring is full, write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
        return false;
    }

    DEBUG("ntp_msgcopy start with write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
    memset(self->shmring->buf[w_idx], 0, BUF_SIZE);
    memcpy(self->shmring->buf[w_idx], element, ele_len);
    DEBUG("[after push]value=%d", *(int *)self->shmring->buf[w_idx]);

    nt_atomic_store64_explicit(&self->shmring->write_index,
                               w_next_idx, ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("push nts shmring successfully!");

    return true;
}

bool shmring_pop(shmring_handle_t self, char *element, size_t ele_len) {
    assert(self);
    assert(element);
    DEBUG("ntp_shmring_pop start with shmaddr='%s'.", self->shm_addr);

    uint64_t w_idx = nt_atomic_load64_explicit(&self->shmring->write_index, ATOMIC_MEMORY_ORDER_ACQUIRE);
    uint64_t r_idx = nt_atomic_load64_explicit(&self->shmring->read_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Queue is empty (or was empty when we checked)
    if (empty(w_idx, r_idx))
        return false;

    ele_len = ele_len > BUF_SIZE ? BUF_SIZE : ele_len;

    DEBUG("ntp_msgcopy start with write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
    memset(element, 0, ele_len);
    memcpy(element, self->shmring->buf[self->shmring->read_index], ele_len);
    DEBUG("[pop]value=%d", *(int *)self->shmring->buf[self->shmring->read_index]);

    nt_atomic_store64_explicit(&self->shmring->read_index,
                               mask_increment(r_idx, self->MASK), ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("pop shmring successfully!");

    return true;

}

bool shmring_front(shmring_handle_t self, char *element, size_t ele_len) {
    assert(self);
    assert(element);

    ele_len = ele_len > BUF_SIZE ? BUF_SIZE : ele_len;

    uint64_t w_idx = nt_atomic_load64_explicit(&self->shmring->write_index, ATOMIC_MEMORY_ORDER_ACQUIRE);
    uint64_t r_idx = nt_atomic_load64_explicit(&self->shmring->read_index, ATOMIC_MEMORY_ORDER_RELAXED);
    
    DEBUG("shmring_front write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
   /// Queue is empty (or was empty when we checked)
   if (empty(w_idx, r_idx)){
    //    INFO("shmring is empty!!!");
       return false;
   }
       

    DEBUG("ntp_msgcopy start with write_idx=%d, read_idx=%d", (int)w_idx, (int)r_idx);
    memset(element, 0, ele_len);
    memcpy(element, self->shmring->buf[self->shmring->read_index], ele_len);
    DEBUG("[front]value=%d", *(int *)self->shmring->buf[self->shmring->read_index]);


    DEBUG("front shmring successfully!");

    return true;
}

void shmring_free(shmring_handle_t self, int unlink) {
    assert(self);
    DEBUG("nts shmring free start");

    munmap(self->shmring, sizeof(struct shmring_buf));
    close(self->shm_fd);
    DEBUG("munmap close pass");

    if (unlink) {
        shm_unlink(self->shm_addr);
    }

    free(self);
    DEBUG("free shmring successfully!");
}


uint64_t ntp_get_read_index(shmring_handle_t self)
{
    const uint64_t r_idx = nt_atomic_load64_explicit(
        &self->shmring->read_index, ATOMIC_MEMORY_ORDER_RELAXED);
    return r_idx;
}

uint64_t ntp_get_peer_read_index(shmring_handle_t self)
{
    const uint64_t op_idx = nt_atomic_load64_explicit(
        &self->peer_read_index, ATOMIC_MEMORY_ORDER_RELAXED);
    return op_idx;
}

int ntp_set_peer_read_index(shmring_handle_t self, uint64_t read_index)
{
    nt_atomic_store64_explicit(&self->peer_read_index,
                               read_index, ATOMIC_MEMORY_ORDER_RELAXED);
    return 0;
}
