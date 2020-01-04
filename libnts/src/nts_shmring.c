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

DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

typedef struct nts_shmring_buf {
//    char buf[NTS_MAX_BUFS + 1][NTS_BUF_SIZE];
	nts_msg buf[NTS_MAX_BUFS + 1];
    uint64_t write_index;
    uint64_t read_index;
} nts_shmring_buf;

typedef struct _nts_shmring {
    int shm_fd;
    unsigned long MASK;
    struct nts_shmring_buf *shmring;
    uint64_t max_size;
} _nts_shmring;


static void error(char *msg);


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
nts_shmring_handle_t nts_shmring_init() {
    nts_shmring_handle_t shmring_handle;

    shmring_handle = (nts_shmring_handle_t) malloc(sizeof(nts_shmring_t));
    DEBUG("init nts_shmring");

    // get shared memory for nts_shmring
    shmring_handle->shm_fd = shm_open(NTS_SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (shmring_handle->shm_fd == -1) {
        if (errno == ENOENT) {
            shmring_handle->shm_fd = shm_open(NTS_SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (shmring_handle->shm_fd == -1) {
                error("shm_open");
                goto FAIL;
            }
        } else {
            error("shm_open");
            goto FAIL;
        }
    }
    DEBUG("shm_open pass");

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
    DEBUG("mmap pass");

    shmring_handle->MASK = NTS_MAX_BUFS - 1;
    shmring_handle->max_size = NTS_MAX_BUFS;
    DEBUG("nts shmring init successfully!");


    return shmring_handle;

    FAIL:
    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);
        shm_unlink(NTS_SHM_NAME);
    }

    free(shmring_handle);
    return NULL;
}


nts_shmring_handle_t nts_get_shmring() {
    nts_shmring_handle_t shmring_handle;
    DEBUG("nts get shmring start");

    shmring_handle = (nts_shmring_handle_t) malloc(sizeof(nts_shmring_t));

    // get shared memory with specified SHM NAME
    shmring_handle->shm_fd = shm_open(NTS_SHM_NAME, O_RDWR, 0);
    if (shmring_handle->shm_fd == -1) {
        error("shm_open");
        goto FAIL;
    }
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
    DEBUG("mmap pass");

    shmring_handle->MASK = NTS_MAX_BUFS - 1;
    shmring_handle->max_size = NTS_MAX_BUFS;
    DEBUG("nts get shmring successfully!");

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
    assert(element);

    const uint64_t w_idx = nt_atomic_load64_explicit(
            &self->shmring->write_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Assuming we write, where will move next ?
    const uint64_t w_next_idx = mask_increment(w_idx, self->MASK);
    DEBUG("\t[nts_shmring] w_next_idx : %d", w_next_idx);

    /// The two pointers colliding means we would have exceeded the
    /// ring buffer size and create an ambiguous state with being empty.
    if (w_next_idx == nt_atomic_load64_explicit(
            &self->shmring->read_index, ATOMIC_MEMORY_ORDER_ACQUIRE))
        return false;

    nts_msgcopy(element, &(self->shmring->buf[w_idx]));
    nt_atomic_store64_explicit(&self->shmring->write_index,
                               w_next_idx, ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("push nts shmring successfully!");

    return true;
}

bool nts_shmring_pop(nts_shmring_handle_t self, nts_msg *element) {
    assert(self);
    assert(element);

    uint64_t w_idx = nt_atomic_load64_explicit(&self->shmring->write_index, ATOMIC_MEMORY_ORDER_ACQUIRE);
    uint64_t r_idx = nt_atomic_load64_explicit(&self->shmring->read_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Queue is empty (or was empty when we checked)
    if (empty(w_idx, r_idx))
        return -1;

    nts_msgcopy(&(self->shmring->buf[self->shmring->read_index]), element);
    nt_atomic_store64_explicit(&self->shmring->read_index,
                               mask_increment(r_idx, self->MASK), ATOMIC_MEMORY_ORDER_RELEASE);

    DEBUG("pop nts shmring successfully!");

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
        shm_unlink(NTS_SHM_NAME);
    }

    free(self);
    DEBUG("free nts shmring successfully!");
}

/**
 * Print system error and exit
 * @param msg
 */
void error(char *msg) {
    perror(msg);
}
