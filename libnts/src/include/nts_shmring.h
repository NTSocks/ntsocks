#ifndef NTS_SHMRING_H_
#define NTS_SHMRING_H_

#include <stdbool.h>
#include <stdio.h>

#include "nts_msg.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NTS_MAX_BUFS 16 // define NTS_MAX_BUFS as [(a power of 2) -1] (65535 in our case)
#define NTS_BUF_SIZE 256

#define NTS_SHM_NAME "/nts-shm-ring"

    typedef struct _nts_shmring nts_shmring_t;
    typedef nts_shmring_t *nts_shmring_handle_t;

    nts_shmring_handle_t nts_shmring_init(char *shm_addr, size_t addrlen);

    nts_shmring_handle_t nts_get_shmring(char *shm_addr, size_t addrlen);

    bool nts_shmring_push(nts_shmring_handle_t self, nts_msg *element);

    bool nts_shmring_pop(nts_shmring_handle_t self, nts_msg *element);

    void nts_shmring_free(nts_shmring_handle_t self, int unlink);

#ifdef __cplusplus
};
#endif

#endif /* NTS_SHMRING_H_ */
