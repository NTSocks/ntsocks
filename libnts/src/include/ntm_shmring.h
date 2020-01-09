//
// Created by spark2 on 12/6/19.
//

#ifndef NTM_SHMRING_H
#define NTM_SHMRING_H

#include <stdbool.h>
#include <stdio.h>

#include "nts_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NTM_MAX_BUFS 1024 // define NTM_MAX_BUFS as a power of 2 (65536 in our case)
#define NTM_BUF_SIZE 256

#define NTM_SEM_MUTEX_NAME "/ntm-sem-mutex"
#define NTM_SEM_BUF_COUNT_NAME "/ntm-sem-buf-count"
#define NTM_SEM_SPOOL_SIGNAL_NAME "/ntm-sem-spool-signal"
#define NTM_SHM_NAME "/ntm-shm-ring"


typedef struct _ntm_shmring ntm_shmring_t;
typedef ntm_shmring_t* ntm_shmring_handle_t;


ntm_shmring_handle_t ntm_shmring_init(char *shm_addr, size_t addrlen);

ntm_shmring_handle_t ntm_get_shmring(char *shm_addr, size_t addrlen);

bool ntm_shmring_push(ntm_shmring_handle_t self, ntm_msg *element);

bool ntm_shmring_pop(ntm_shmring_handle_t self, ntm_msg *element);

void ntm_shmring_free(ntm_shmring_handle_t self, int unlink);

#ifdef __cplusplus
};
#endif

#endif //NTM_SHMRING_H
