/*
 * <p>Title: nts_shmring.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 12, 2019 
 * @version 1.0
 */

#ifndef NTS_SHMRING_H_
#define NTS_SHMRING_H_

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NTS_MAX_BUFS 1024 // define NTS_MAX_BUFS as [(a power of 2) -1] (65535 in our case)
#define NTS_BUF_SIZE 256

#define NTS_SHM_NAME "/nts-shm-ring"

#define NTS_LIKELY(x) __builtin_expect(!!(x), 1)
#define NTS_UNLIKELY(x) __builtin_expect(!!(x), 0)

typedef struct _nts_shmring nts_shmring_t;
typedef nts_shmring_t* nts_shmring_handle_t;


nts_shmring_handle_t nts_shmring_init();

nts_shmring_handle_t nts_get_shmring();

bool nts_shmring_push(nts_shmring_handle_t self, char *element, size_t len);

int nts_shmring_pop(nts_shmring_handle_t self, char *element, size_t len);

void nts_shmring_free(nts_shmring_handle_t self, int unlink);

#ifdef __cplusplus
};
#endif

#endif /* NTS_SHMRING_H_ */
