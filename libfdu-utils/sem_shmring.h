#ifndef __SEM_SHMRING__H__
#define __SEM_SHMRING__H__

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SHMRING_MAX_BUFS 8  //  a power of 2 
#define SHMRING_BUF_SIZE 256
#define SEM_MUTEX_PREFIX "/mutex-"
#define SEM_BUF_COUNT_PREFIX "/buf-count-"
#define SEM_SPOOL_SIGNAL_PREFIX "/spool-signal-"
#define SHM_RING_PREFIX "/sem_shmring-"

typedef struct _sem_shmring sem_shmring_t;
typedef sem_shmring_t *sem_shmring_handle_t;

typedef enum _sem_shmring_stat {
    SEM_SHMRING_UNREADY = 0,
    SEM_SHMRING_READY,
    SEM_SHMRING_CLOSE,
    SEM_SHMRING_UNLINK
} sem_shmring_stat;

// When master first create sem_shmring
sem_shmring_handle_t sem_shmring_create(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num);

// When slave first try to init/connect existing sem_shmring
sem_shmring_handle_t sem_shmring_init(char *shm_addr, size_t addrlen, size_t ele_size, size_t ele_num);

// Returns 0 on success, -1 if buffer is full
int sem_shmring_push(sem_shmring_handle_t handle, char *element, size_t ele_len);

//  Returns 0 on success, -1 if the buffer is empty
int sem_shmring_pop(sem_shmring_handle_t handle, char *element, size_t ele_len);

//  Returns 0 on success, -1 if the buffer is empty
int sem_shmring_front(sem_shmring_handle_t handle, char *element, size_t ele_len);

// Free sem_shmring structure, if unlink is 1, unlink all SHM-based variables.
int sem_shmring_free(sem_shmring_handle_t handle, bool is_unlink);

// Reset the sem_shmring circular buffer to empty, head == tail
int sem_shmring_reset(sem_shmring_handle_t handle);

// Returns true if the buffer is empty
bool sem_shmring_empty(sem_shmring_handle_t handle);

// Returns true if the buffer is full
bool sem_shmring_full(sem_shmring_handle_t handle);

// Returns the maximum capacity of the buffer
size_t sem_shmring_capacity(sem_shmring_handle_t handle);

// Returns the current number of elements in the buffer
size_t sem_shmring_size(sem_shmring_handle_t handle);

#ifdef __cplusplus
};
#endif

#endif  //!__SEM_SHMRING__H__