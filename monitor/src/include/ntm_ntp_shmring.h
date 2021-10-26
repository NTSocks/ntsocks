#ifndef NTM_NTP_SHMRING_H_
#define NTM_NTP_SHMRING_H_

#include <stdbool.h>
#include <stdio.h>

#include "ntm_ntp_msg.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _ntm_ntp_shmring ntm_ntp_shmring_t;
    typedef ntm_ntp_shmring_t *ntm_ntp_shmring_handle_t;

    ntm_ntp_shmring_handle_t ntm_ntp_shmring_init(char *shm_addr, size_t addrlen);

    ntm_ntp_shmring_handle_t ntm_ntp_get_shmring(char *shm_addr, size_t addrlen);

    bool ntm_ntp_shmring_push(ntm_ntp_shmring_handle_t self, ntm_ntp_msg *element);

    bool ntm_ntp_shmring_pop(ntm_ntp_shmring_handle_t self, ntm_ntp_msg *element);

    void ntm_ntp_shmring_free(ntm_ntp_shmring_handle_t self, int unlink);

#ifdef __cplusplus
};
#endif

#endif /* NTM_NTP_SHMRING_H_ */
