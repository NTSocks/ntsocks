//
// Created by spark2 on 12/6/19.
//
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#include "ntm_shmring.h"
#include "nt_log.h"


DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

typedef struct ntm_shmring_buf {
//    char buf[NTM_MAX_BUFS][NTM_BUF_SIZE];
	// ntm_msg buf[NTM_MAX_BUFS + 1];
    ntm_msg buf[NTM_MAX_BUFS];
    unsigned long write_index;
    unsigned long read_index;
} ntm_shmring_buf;

typedef struct _ntm_shmring {
    sem_t *mutex_sem;
    sem_t *buf_count_sem;
    sem_t *spool_signal_sem;

    int shm_fd;
    unsigned long MASK;
    int addrlen;
    char *shm_addr;
    struct ntm_shmring_buf *shmring;
} _ntm_shmring;


static void error(const char *msg);

/**
 * invoked by monitor process.
 * init the ntm_shmring by:
 *  1. register shm for data buffer;
 *  2. register shm-based semaphore and mutex for sync status
 *     between monitor and libnts apps processes;
 *  3. init the read and write index for shm-based data buffer.
 * @return
 */
ntm_shmring_handle_t ntm_shmring_init(char *shm_addr, size_t addrlen) {
    assert(shm_addr);
    assert(addrlen > 0);

	ntm_shmring_handle_t shmring_handle;

    shmring_handle = (ntm_shmring_handle_t) malloc(sizeof(ntm_shmring_t));
	DEBUG("init ntm_shmring with shm_addr=%s", shm_addr);
	shmring_handle->addrlen = addrlen;
	shmring_handle->shm_addr = shm_addr;

    // get shared memory for ntm_shmring
    shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR | O_CREAT, 0666);    ///???
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
	DEBUG("shm_open pass");

    ///???
    int ret;
    ret = ftruncate(shmring_handle->shm_fd, sizeof(struct ntm_shmring_buf));
    if (ret == -1) {
        error("ftruncate");
        goto FAIL;
    }
	DEBUG("ftruncate pass");

    // mmap the allocated shared memory to ntm_shmring
    shmring_handle->shmring = (struct ntm_shmring_buf *)
            mmap(NULL, sizeof(struct ntm_shmring_buf),
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shmring_handle->shm_fd, 0);
    if (shmring_handle->shmring == MAP_FAILED) {
        error("mmap");
        goto FAIL;
    }
    close(shmring_handle->shm_fd);
    shmring_handle->shm_fd = -1;
    // init the shared memory
    shmring_handle->shmring->read_index = shmring_handle->shmring->write_index = 0;
	DEBUG("mmap pass");

    // store old, umask for world-writable access of semaphores (mutex_sem, buf_count_sem, spool_signal_sem)
    mode_t old_umask;
    old_umask = umask(0);

    // mutual exclusion semaphore, mutex_sem with an initial value 0.
    shmring_handle->mutex_sem = sem_open(NTM_SEM_MUTEX_NAME, O_CREAT, 0666, 0);
    if (shmring_handle->mutex_sem == SEM_FAILED) {
        error("sem_open: mutex_sem");
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open mutex_sem pass");

    // counting semaphore, indicating the number of available buffers in shm ring.
    // init value = NTM_MAX_BUFS
    shmring_handle->buf_count_sem = sem_open(NTM_SEM_BUF_COUNT_NAME, O_CREAT, 0666, NTM_MAX_BUFS);
    if ((sem_t *)shmring_handle->buf_count_sem == SEM_FAILED) {
        error("sem_open: buf_count_sem");
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open buf_count_sem pass");

    // count semaphore, indicating the number of data buffers to be read. Init value = 0
    shmring_handle->spool_signal_sem = sem_open(NTM_SEM_SPOOL_SIGNAL_NAME, O_CREAT, 0666, 0);
    if (shmring_handle->spool_signal_sem == SEM_FAILED) {
        error("sem_open: spool_signal_sem");
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open spool_signal_sem pass");

    // restore old mask
    umask(old_umask);

    // init complete; now set mutex semaphore as 1 to
    // indicate the shared memory segment is available
    ret = sem_post(shmring_handle->mutex_sem);
    if (ret == -1) {
        error("sem_post: mutex_sem");
        goto FAIL;
    }

    shmring_handle->MASK = NTM_MAX_BUFS - 1;
	DEBUG("ntm shmring init successfully!");

    return shmring_handle;

    FAIL:
    if (shmring_handle->spool_signal_sem != SEM_FAILED) {
        sem_close(shmring_handle->spool_signal_sem);
        sem_unlink(NTM_SEM_SPOOL_SIGNAL_NAME);
    }
    if (shmring_handle->buf_count_sem != SEM_FAILED) {
        sem_close(shmring_handle->buf_count_sem);
        sem_unlink(NTM_SEM_BUF_COUNT_NAME);
    }

    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);
        shm_unlink(shmring_handle->shm_addr);
    }
    if (shmring_handle->mutex_sem != SEM_FAILED) {
        sem_close(shmring_handle->mutex_sem);
        sem_unlink(NTM_SEM_MUTEX_NAME);
    }

    free(shmring_handle);
    return NULL;
}


ntm_shmring_handle_t ntm_get_shmring(char *shm_addr, size_t addrlen) {
	assert(shm_addr);
	assert(addrlen > 0);

    ntm_shmring_handle_t shmring_handle;
	DEBUG("ntm get shmring start");

    shmring_handle = (ntm_shmring_handle_t) malloc(sizeof(ntm_shmring_t));
    DEBUG("init ntm_shmring with shm_addr=%s", shm_addr);
    shmring_handle->addrlen = addrlen;
    shmring_handle->shm_addr = shm_addr;


    // get shared memory with specified SHM NAME
    shmring_handle->shm_fd = shm_open(shmring_handle->shm_addr, O_RDWR, 0); ///???
    if (shmring_handle->shm_fd == -1) {
        error("shm_open");
        goto FAIL;
    }
    // set the permission of shm fd for write/read in non-root users 
    fchmod(shmring_handle->shm_fd, 0666);
	DEBUG("shm_open pass with fd - %d", shmring_handle->shm_fd);

    // mmap the allocated shared memory to ntm_shmring
    shmring_handle->shmring = (struct ntm_shmring_buf *)
            mmap(NULL, sizeof(struct ntm_shmring_buf),
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shmring_handle->shm_fd, 0);
    if (shmring_handle->shmring == MAP_FAILED) {
        error("mmap");
        goto FAIL;
    }
    close(shmring_handle->shm_fd);
    shmring_handle->shm_fd = -1;


    // store old, umask for world-writable access of semaphores (mutex_sem, buf_count_sem, spool_signal_sem)
    mode_t old_umask;
    old_umask = umask(0);

    // mutual exclusion semaphore, mutex_sem
    shmring_handle->mutex_sem = sem_open(NTM_SEM_MUTEX_NAME, O_CREAT, 0666, 0);
    if (shmring_handle->mutex_sem == SEM_FAILED) {
        error("sem_open: mutex_sem");
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open mutex_sem pass");

    // count semaphore. indicate the number of available shm segments to be written.
    shmring_handle->buf_count_sem = sem_open(NTM_SEM_BUF_COUNT_NAME, O_CREAT, 0666, NTM_MAX_BUFS);
    if (shmring_handle->buf_count_sem == SEM_FAILED) {
        error("sem_open: buf_count_sem");
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open buf count_sem pass");

    // count semaphore. indicate the number of shm segments to be read.
    shmring_handle->spool_signal_sem = sem_open(NTM_SEM_SPOOL_SIGNAL_NAME, O_CREAT, 0666, 0);
    if (shmring_handle->spool_signal_sem == SEM_FAILED) {
        error("sem_open: spool_signal_sem");
        umask(old_umask);
        goto FAIL;
    }
	DEBUG("sem_open spool_signal_sem pass");

    // restore old mask
    umask(old_umask);

    shmring_handle->MASK = NTM_MAX_BUFS - 1;
	DEBUG("ntm get shmring successfully!");

    return shmring_handle;

    FAIL:
    if (shmring_handle->spool_signal_sem != SEM_FAILED) {
        sem_close(shmring_handle->spool_signal_sem);
    }
    if (shmring_handle->buf_count_sem != SEM_FAILED) {
        sem_close(shmring_handle->buf_count_sem);
    }

    if (shmring_handle->shm_fd != -1) {
        close(shmring_handle->shm_fd);
    }
    if (shmring_handle->mutex_sem != SEM_FAILED) {
        sem_close(shmring_handle->mutex_sem);
    }

    free(shmring_handle);
    return NULL;
}

/**
 * push an element into shm-based ring buffer
 * @param self
 * @param element
 * @return
 */
bool ntm_shmring_push(ntm_shmring_handle_t self, ntm_msg *element) {
    assert(self);
    assert(element);
    int rc;

    // get a buffer: P (buf_count_sem)
    rc = sem_wait(self->buf_count_sem);
    if (rc == -1) {
        error("sem_wait: buf_count_sem");
        goto FAIL;
    }

    /**
     * There might be multiple producers. We must ensure that
     * only one producer uses write_index at a time.
     */
    rc = sem_wait(self->mutex_sem);
    if (rc == -1) {
        error("sem_wait: mutex_sem");
        goto FAIL;
    }
	DEBUG("enter critical section... ");

    /* Critical Section */
	ntm_msgcopy(element, &(self->shmring->buf[self->shmring->write_index]));
    (self->shmring->write_index)++;
    // self->shmring->write_index &= self->MASK;
    if (self->shmring->write_index >= NTM_MAX_BUFS) {
        self->shmring->write_index = self->shmring->write_index - NTM_MAX_BUFS;   
    }

    // Release mutex sem: V (mutex_sem)
    rc = sem_post(self->mutex_sem);
    if (rc == -1) {
        error("sem_post: mutex_sem");
        goto FAIL;
    }

    // tell spooler that there is a element to read: V (spool_signal_sem)
    rc = sem_post(self->spool_signal_sem);
    if (rc == -1) {
        error("sem_post: spool_signal_sem");
        goto FAIL;
    }

	DEBUG("push ntm shmring successfully!");
    return true;

    FAIL:
    return false;
}

bool ntm_shmring_pop(ntm_shmring_handle_t self, ntm_msg *element) {
    assert(self);
    if(!element) {
        DEBUG("element not existing.");
    }

    int rc;

    // Is there a shm element to read ? P(spool_signal_sem)
    rc = sem_wait(self->spool_signal_sem);
    if (rc == -1) {
        error("sem_wait: spool_signal_sem");
        goto FAIL;
    }

    
    DEBUG("ntm_write_index=%ld, ntm_read_index=%ld", self->shmring->write_index, self->shmring->read_index);
	ntm_msgcopy(&(self->shmring->buf[self->shmring->read_index]), element);

    /**
     * Since there is only one process (nt-monitor) using the
     * read_index, mutex semaphore is not necessary
     */
    (self->shmring->read_index)++;
    self->shmring->read_index &= self->MASK;
    // if (self->shmring->write_index >= NTM_MAX_BUFS) {
    //     self->shmring->write_index = self->shmring->write_index - NTM_MAX_BUFS;   
    // }

    /**
     * one element of shm ring has been pop.
     * One more buffer is available for push by producers (libnts app).
     * Release buffer: V (buf_count_sem);
     */
    rc = sem_post(self->buf_count_sem);
    if (rc == -1) {
        error("sem_post: buf_count_sem");
        goto FAIL;
    }
	DEBUG("pop ntm shmring successfully!");
    return true;

    FAIL:
    return false;
}

/**
 * free ntm shmring when nt-monitor process exit.
 * @param self
 * @param unlink
 */
void ntm_shmring_free(ntm_shmring_handle_t self, int unlink) {
    assert(self);
	DEBUG("ntm shmring free start");

    sem_close(self->mutex_sem);

    munmap(self->shmring, sizeof(struct ntm_shmring_buf));

    sem_close(self->buf_count_sem);
    sem_close(self->spool_signal_sem);

    if (unlink) {
        sem_unlink(NTM_SEM_MUTEX_NAME);
        shm_unlink(self->shm_addr);
        sem_unlink(NTM_SEM_BUF_COUNT_NAME);
        sem_unlink(NTM_SEM_SPOOL_SIGNAL_NAME);
    }

    free(self);
	DEBUG("free ntm shmring successfully!");
}

/**
 * Print system error and exit
 * @param msg
 */
void error(const char *msg) {
    perror(msg);
}
