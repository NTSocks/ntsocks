/*
 * <p>Title: ntp_nts_shmring.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 12, 2019 
 * @version 1.0
 */

#ifndef NTP_NTS_SHMRING_H_
#define NTP_NTS_SHMRING_H_

#include <stdbool.h>
#include <stdio.h>

#include "ntp_nts_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NTS_MAX_BUFS 1024 // define NTS_MAX_BUFS as [(a power of 2) -1] (65535 in our case)
#define NTS_BUF_SIZE 256

#define NTP_SHM_NAME "/nts-shm-ring"

#define NTS_LIKELY(x) __builtin_expect(!!(x), 1)
#define NTS_UNLIKELY(x) __builtin_expect(!!(x), 0)

typedef struct _ntp_shmring ntp_shmring_t;
typedef ntp_shmring_t* ntp_shmring_handle_t;

uint64_t ntp_get_read_index(ntp_shmring_handle_t shmring_handle);

uint64_t ntp_get_opposide_readindex(ntp_shmring_handle_t shmring_handle);

uint32_t ntp_get_detect_pkg_state(ntp_shmring_handle_t shmring_handle);

int ntp_set_detect_pkg_state(ntp_shmring_handle_t shmring_handle, uint32_t val);

int ntp_set_opposide_readindex(ntp_shmring_handle_t shmring_handle,uint64_t read_index);

ntp_shmring_handle_t ntp_shmring_init(char *shm_addr, size_t addrlen);

ntp_shmring_handle_t ntp_get_shmring(char *shm_addr, size_t addrlen);

bool ntp_shmring_push(ntp_shmring_handle_t self, ntp_msg *element);

bool ntp_shmring_pop(ntp_shmring_handle_t self, ntp_msg *element);

void ntp_shmring_free(ntp_shmring_handle_t self, int unlink);

#ifdef __cplusplus
};
#endif

#endif /* NTP_NTS_SHMRING_H_ */
