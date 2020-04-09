/*
 * <p>Title: nts_shmring.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 12, 2019 
 * @version 1.0
 */

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

#include "nts_shmring.h"
#include "nt_atomic.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

// #define NTS_SHM_TEST "nts-test-shm"

typedef struct nts_shmring_buf {
//    char buf[NTS_MAX_BUFS + 1][NTS_BUF_SIZE];
    nts_msg buf[NTS_MAX_BUFS + 1];
    uint64_t write_index;
    uint64_t read_index;
    
} nts_shmring_buf;

typedef struct _nts_shmring {
    int shm_fd;
    unsigned long MASK;
    int addrlen;
    char *shm_addr;

    struct nts_shmring_buf *shmring;
    uint64_t max_size;
} _nts_shmring;


static void error(const char *msg);


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


/**
 * invoked by monitor process.
 * init the nts_shmring by:
 *  1. register shm for data buffer;
 *  2. register shm-based semaphore for sync status
 *     between monitor and libnts apps processes;
 *  3. init the read and write index for shm-based data buffer.
 * @return
 */
nts_shmring_handle_t nts_shmring_init(char *shm_addr, size_t addrlen) {
	assert(shm_addr);
	assert(addrlen > 0);

    nts_shmring_handle_t shmring_handle;

    shmring_handle = (nts_shmring_handle_t) malloc(sizeof(nts_shmring_t));
    DEBUG("init nts_shmring with shm_addr=%s", shm_addr);
    shmring_handle->addrlen = addrlen;
    shmring_handle->shm_addr = shm_addr;

    // get shared memory for nts_shmring
    shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR | O_CREAT, 0666);
    if (shmring_handle->shm_fd == -1) {
        if (errno == ENOENT || errno == EEXIST) {
            shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR | O_CREAT, 0666);
            if (shmring_handle->shm_fd == -1) {
                error("shm_open");
                goto FAIL;
            }
        } else {
            error("shm_open");
            goto FAIL;
        }
    }
    // set the permission of shm fd for write/read in non-root users 
    fchmod(shmring_handle->shm_fd, 0666);
    DEBUG("shm_open pass with fd - %d", shmring_handle->shm_fd);

    int ret;
    ret = ftruncate(shmring_handle->shm_fd, sizeof(struct nts_shmring_buf));
    if (ret == -1) {
        error("ftruncate");
        goto FAIL;
    }
    DEBUG("ftruncate pass");

    // mmap the allocated shared memory to nts_shmring
    shmring_handle->shmring = (struct nts_shmring_buf *)
            mmap(NULL, sizeof(struct nts_shmring_buf),
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shmring_handle->shm_fd, 0);
    if (shmring_handle->shmring == MAP_FAILED) {
        error("mmap");
        goto FAIL;
    }
    // init the shared memory
    shmring_handle->shmring->read_index = shmring_handle->shmring->write_index = 0;
    DEBUG("mmap pass with read_index=%ld, write_index=%ld", shmring_handle->shmring->read_index,  shmring_handle->shmring->write_index);

    shmring_handle->MASK = NTS_MAX_BUFS - 1;
    shmring_handle->max_size = NTS_MAX_BUFS;
    DEBUG("nts shmring init success!");


    return shmring_handle;

    FAIL:
    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);
        shm_unlink(shmring_handle->shm_addr);
    }

    free(shmring_handle);
    return NULL;
}


nts_shmring_handle_t nts_get_shmring(char *shm_addr, size_t addrlen) {
	assert(shm_addr);
	assert(addrlen > 0);

    nts_shmring_handle_t shmring_handle;
    DEBUG("nts get shmring start with shm_addr=%s", shm_addr);

    shmring_handle = (nts_shmring_handle_t) malloc(sizeof(nts_shmring_t));
    memset(shmring_handle, 0, sizeof(nts_shmring_t));
    shmring_handle->addrlen = addrlen;
    shmring_handle->shm_addr = shm_addr;

    // get shared memory with specified SHM NAME
    shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR, 0);
    if (shmring_handle->shm_fd == -1) {
        error("shm_open");
        goto FAIL;
    }
    // set the permission of shm fd for write/read in non-root users 
    fchmod(shmring_handle->shm_fd, 0666);
    DEBUG("shm_open pass with fd - %d", shmring_handle->shm_fd);

    // mmap the allocated shared memory to nts_shmring
    shmring_handle->shmring = (struct nts_shmring_buf *)
            mmap(NULL, sizeof(struct nts_shmring_buf),
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shmring_handle->shm_fd, 0);
    if (shmring_handle->shmring == MAP_FAILED) {
        error("mmap");
        goto FAIL;
    }
    DEBUG("mmap pass with read_index=%ld, write_index=%ld", shmring_handle->shmring->read_index,  shmring_handle->shmring->write_index);

    shmring_handle->MASK = NTS_MAX_BUFS - 1;
    shmring_handle->max_size = NTS_MAX_BUFS;
    DEBUG("nts get shmring success!");

    return shmring_handle;

    FAIL:
    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);
    }

    free(shmring_handle);
    return NULL;
}

/**
 * push an element into shm-based ring buffer
 * @param self
 * @param element
 * @param len
 * @return
 */
bool nts_shmring_push(nts_shmring_handle_t self, nts_msg *element) {
    assert(self);
    assert(self->shmring);
    assert(element);

    /* Critical Section */
    /*const uint64_t r_idx = nt_atomic_load64_explicit(
            &self->shmring->read_index, ATOMIC_MEMORY_ORDER_CONSUME);*/
//    const uint64_t w_idx = nt_atomic_load64_explicit(
//            &self->shmring->write_index, ATOMIC_MEMORY_ORDER_CONSUME);

    DEBUG("self->shmring->write_index=%ld, self->shmring->read_index=%ld", 
                    self->shmring->write_index, self->shmring->read_index);
    const uint64_t w_idx = nt_atomic_load64_explicit(
            &self->shmring->write_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Assuming we write, where will move next ?
    const uint64_t w_next_idx = mask_increment(w_idx, self->MASK);

    /// The two pointers colliding means we would have exceeded the
    /// ring buffer size and create an ambiguous state with being empty.
    if (w_next_idx == nt_atomic_load64_explicit(
            &self->shmring->read_index, ATOMIC_MEMORY_ORDER_ACQUIRE))
        return false;

    nts_msgcopy(element, &(self->shmring->buf[w_idx]));
    nt_atomic_store64_explicit(&self->shmring->write_index,
                               w_next_idx, ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("push nts shmring success!");

    return true;

}

bool nts_shmring_pop(nts_shmring_handle_t self, nts_msg *element) {
    assert(self);
    assert(element);
    assert(self->shmring);

    uint64_t w_idx = nt_atomic_load64_explicit(&self->shmring->write_index, ATOMIC_MEMORY_ORDER_ACQUIRE);
    uint64_t r_idx = nt_atomic_load64_explicit(&self->shmring->read_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Queue is empty (or was empty when we checked)
    if (empty(w_idx, r_idx))
        return false;

    nts_msgcopy(&(self->shmring->buf[self->shmring->read_index]), element);
    nt_atomic_store64_explicit(&self->shmring->read_index,
                               mask_increment(r_idx, self->MASK), ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("pop nts shmring success!");

    return true;

}

/**
 * free nts shmring when nt-monitor process exit.
 * @param self
 * @param unlink
 */
void nts_shmring_free(nts_shmring_handle_t self, int unlink) {
    assert(self);
    DEBUG("nts shmring free start");

    munmap(self->shmring, sizeof(struct nts_shmring_buf));
    close(self->shm_fd);
    DEBUG("munmap close pass");

    if (unlink) {
        shm_unlink(self->shm_addr);
    }

    free(self);
    DEBUG("free nts shmring success!");
}

/**
 * Print system error and exit
 * @param msg
 */
void error(const char* msg) {
    perror(msg);
}
