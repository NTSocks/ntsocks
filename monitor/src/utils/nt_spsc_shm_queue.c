/*
 * <p>Title: nt_spsc_shm_queue.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Jan 6, 2020 
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "nt_spsc_shm_queue.h"
#include "nt_atomic.h"

// the address index of read_index and write_index
#define RD_IDX_ADDR_IDX             0
#define WR_IDX_ADDR_IDX             (1 * sizeof(uint64_t))
#define DATA_START_ADDR_IDX         (2 * sizeof(uint64_t))


// | read_index(8 bytes) | write_index(8 bytes) | data((element_num + 1) * element_size) |
typedef struct _nt_spsc_shmring {
    char *shmbuf;               // the address pointer of shm buffer
    uint64_t *write_index;      // the address pointer of write_index for the shm ring buffer
    uint64_t *read_index;       // the address pointer of read_index for the shm ring buffer
    char *data;                 // the start address pointer of shm data (element)

    size_t element_size;   // the size of each element
    int element_num;    // the total number capacity of element
    size_t total_shm_size; // the total size of shm buffer, including: read_index, write_index, data

    int shm_fd;         // the fd of shm
    uint64_t MASK;
    int addrlen;        // the length of shm name
    char *shm_addr;     // the shm name to index the shm ring buffer

} _nt_spsc_shmring;


static void error(const char *msg);

static inline uint64_t increment(uint64_t current_idx, uint64_t max_size) {

    return (current_idx + 1) % max_size;
}

static inline uint64_t next_index(uint64_t current_idx, uint64_t max_size) {
    uint64_t ret = current_idx + 1;
    while (NT_UNLIKELY(ret >= max_size))
        ret -= max_size;

    return ret;
}


static inline uint64_t mask_increment(uint64_t current_idx, uint64_t mask) {

    return (current_idx + 1) & mask;
}

static inline bool empty(uint64_t write_index, uint64_t read_index) {
    return write_index == read_index;
}

/**
 *
 * @param shm_addr
 * @param addrlen
 * @param element_num
 * @param element_size
 * @param is_master : if it is invoked by nt_spsc_shmring_init, `is_master` is true; else, `is_master` is false
 * @return
 */
static inline nt_spsc_shmring_handle_t _nt_spsc_shmring_init(char *shm_addr, size_t addrlen, int element_num, size_t element_size, bool is_master) {
    assert(shm_addr);
    assert(addrlen > 0);
    assert(element_num > 0);
    assert(element_size > 0);

    nt_spsc_shmring_handle_t shmring_handle;
    shmring_handle = (nt_spsc_shmring_handle_t) malloc(sizeof(nt_spsc_shmring_t));
    memset(shmring_handle, 0, sizeof(nt_spsc_shmring_t));
    shmring_handle->addrlen = (int) addrlen;
    shmring_handle->shm_addr = shm_addr;
    shmring_handle->element_size = element_size;
    shmring_handle->element_num = element_num;

    /**
     * compute the total size of shm buffer:
     *      sizeof(read_index) + sizeof(write_index) + sizeof(element) * (element_num)
     */
    shmring_handle->total_shm_size = sizeof(uint64_t) * 2 + element_size * element_num;

    // get shared memory for nts_shmring
    if (is_master) {
        shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    } else {
        shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR, 0);
    }

    if (shmring_handle->shm_fd == -1) {
        if (is_master && errno == ENOENT) {
            shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (shmring_handle->shm_fd == -1) {
                error("shm_open");
                goto FAIL;
            }
        } else {
            error("shm_open");
            goto FAIL;
        }
    }

    if (is_master) {
        int ret;
        ret = ftruncate(shmring_handle->shm_fd, shmring_handle->total_shm_size);
        if (ret == -1) {
            error("ftruncate");
            goto FAIL;
        }
    }


    // mmap the allocated shared memory to nts_shmring
    shmring_handle->shmbuf = (char *)
            mmap(NULL, (size_t) shmring_handle->total_shm_size,
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shmring_handle->shm_fd, 0);
    if (shmring_handle->shmbuf == MAP_FAILED) {
        error("mmap");
        goto FAIL;
    }
    shmring_handle->read_index = (uint64_t *) shmring_handle->shmbuf + RD_IDX_ADDR_IDX;
    shmring_handle->write_index = (uint64_t *) (shmring_handle->shmbuf + WR_IDX_ADDR_IDX);
    shmring_handle->data = shmring_handle->shmbuf + DATA_START_ADDR_IDX;
    if (is_master) {
        *(shmring_handle->read_index) = 0;
        *(shmring_handle->write_index) = 0;
        printf("write_index: %ld, read_index: %ld \n", *shmring_handle->write_index, *shmring_handle->read_index);
    }

    shmring_handle->MASK = (uint64_t) (element_num - 1);
    printf("nt shmring init pass\n");

    return shmring_handle;

    FAIL:
    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);

        if(is_master)
            shm_unlink(shmring_handle->shm_addr);
    }

    free(shmring_handle);
    return NULL;
}

/**
 *
 * @param shm_addr
 * @param addrlen
 * @param element_num
 * @param element_size
 * @return
 */
nt_spsc_shmring_handle_t nt_spsc_shmring_init(char *shm_addr, size_t addrlen, int element_num, size_t element_size) {

    return _nt_spsc_shmring_init(shm_addr, addrlen, element_num, element_size, true);
}

/**
 *
 * @param shm_addr
 * @param addrlen
 * @param element_num
 * @param element_size
 * @return
 */
nt_spsc_shmring_handle_t nt_get_spsc_shmring(char *shm_addr, size_t addrlen, int element_num, size_t element_size) {

    return _nt_spsc_shmring_init(shm_addr, addrlen, element_num, element_size, false);
}

/**
 * push an element into shm-based ring buffer
 * @param self
 * @param element
 * @param element_size
 * @return
 */
bool nt_spsc_shmring_push(nt_spsc_shmring_handle_t self, char *element, size_t element_size) {
    assert(self);
    assert(element);
    assert(element_size > 0);

    const uint64_t w_idx = nt_atomic_load64_explicit(
            self->write_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Assuming we write, where will move next ?
    const uint64_t w_next_idx = mask_increment(w_idx, self->MASK);

    /// The two pointers colliding means we would have exceeded the
    /// ring buffer size and create an ambiguous state with being empty.
    if (w_next_idx == nt_atomic_load64_explicit(
            self->read_index, ATOMIC_MEMORY_ORDER_ACQUIRE))
        return false;

    uint64_t current_loc = w_idx * self->element_size;
    memcpy(self->data + current_loc, element, element_size);

    nt_atomic_store64_explicit(self->write_index,
                               w_next_idx, ATOMIC_MEMORY_ORDER_RELEASE);

    printf("push pass\n");

    return true;
}

/**
 * pop an element into shm-based ring buffer
 * @param self
 * @param element
 * @param element_size
 * @return
 */
bool nt_spsc_shmring_pop(nt_spsc_shmring_handle_t self, char *element, size_t element_size) {
    assert(self);
    assert(element);
    assert(element_size > 0);

    element_size > self->element_size ? self->element_size : element_size;

    uint64_t w_idx = nt_atomic_load64_explicit(self->write_index, ATOMIC_MEMORY_ORDER_ACQUIRE);
    uint64_t r_idx = nt_atomic_load64_explicit(self->read_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Queue is empty (or was empty when we checked)
    if (empty(w_idx, r_idx))
        return false;

    uint64_t current_loc = (*self->read_index) * self->element_size;
    memcpy(element, (const void *) self->data + current_loc, (size_t) element_size);

    nt_atomic_store64_explicit(self->read_index, mask_increment(r_idx, self->MASK), ATOMIC_MEMORY_ORDER_RELEASE);
    printf("pop pass\n");


    return true;
}

/**
 * free nt shmring
 * @param self
 * @param unlink
 */
void nt_spsc_shmring_free(nt_spsc_shmring_handle_t self, int unlink) {
    assert(self);

    munmap(self->shmbuf, self->total_shm_size);
    close(self->shm_fd);

    if (unlink) {
        shm_unlink(self->shm_addr);
    }

    free(self);
    printf("free nt spsc shmring successfully!\n");
}


bool nt_spsc_shmring_is_full(nt_spsc_shmring_handle_t self) {
    assert(self);

    const uint64_t w_idx = nt_atomic_load64_explicit(
            self->write_index, ATOMIC_MEMORY_ORDER_RELAXED);

    /// Assuming we write, where will move next ?
    const uint64_t w_next_idx = mask_increment(w_idx, self->MASK);

    /// The two pointers colliding means we would have exceeded the
    /// ring buffer size and create an ambiguous state with being empty.
    if (w_next_idx == nt_atomic_load64_explicit(
            self->read_index, ATOMIC_MEMORY_ORDER_ACQUIRE))
        return true;

    return false;
}


/**
 * Print system error and exit
 * @param msg
 */
void error(const char* msg) {
    perror(msg);
}
