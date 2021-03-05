/*
 * <p>Title: ntp_ntm_shmring.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 12, 2019 
 * @version 1.0
 */

#ifndef NTP_NTM_SHMRING_H_
#define NTP_NTM_SHMRING_H_

#include <stdbool.h>
#include <stdio.h>

#include "ntm_ntp_msg.h"
#include "ntm_ntp_shmring.h"
#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _ntp_ntm_shmring ntp_ntm_shmring_t;
    typedef ntp_ntm_shmring_t *ntp_ntm_shmring_handle_t;

    ntp_ntm_shmring_handle_t ntp_ntm_shmring_init(char *shm_addr, size_t addrlen);

    ntp_ntm_shmring_handle_t ntp_ntm_get_shmring(char *shm_addr, size_t addrlen);

    bool ntp_ntm_shmring_push(ntp_ntm_shmring_handle_t self, ntp_ntm_msg *element);

    bool ntp_ntm_shmring_pop(ntp_ntm_shmring_handle_t self, ntp_ntm_msg *element);

    void ntp_ntm_shmring_free(ntp_ntm_shmring_handle_t self, int unlink);

#ifdef __cplusplus
};
#endif

#endif /* NTP_NTM_SHMRING_H_ */
