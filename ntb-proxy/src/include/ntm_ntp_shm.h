/*
 * <p>Title: ntm_ntp_shm.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTM_NTP_SHM_H_
#define NTM_NTP_SHM_H_

#include <stdio.h>

#include "ntm_ntp_shmring.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum ntm_shm_stat
	{
		NTS_SHM_UNREADY = 0,
		NTS_SHM_READY,
		NTS_SHM_CLOSE,
		NTS_SHM_UNLINK
	} ntm_shm_stat;

	struct ntm_ntp_shm_context
	{
		ntm_shm_stat shm_stat;
		ntm_ntp_shmring_handle_t ntm_ntpring_handle;
		char *shm_addr;
		size_t addrlen;
	};

	typedef struct ntm_ntp_shm_context *ntm_ntp_shm_context_t;

	/**
 * used by libnts app or nt-monitor to create nts_shm_context
 */
	ntm_ntp_shm_context_t ntm_ntp_shm();

	/**
 * used by ntm_ntp shm server (consumer)
 */
	int ntm_ntp_shm_accept(ntm_ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
 * used by nt-monitor client (producer)
 */
	int ntm_ntp_shm_connect(ntm_ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
 * used by nt-monitor to send message to libnts app
 */
	int ntm_ntp_shm_send(ntm_ntp_shm_context_t shm_ctx, ntm_ntp_msg *buf);

	/**
 * used by libnts app to receive message from nt-monitor
 */
	int ntm_ntp_shm_recv(ntm_ntp_shm_context_t shm_ctx, ntm_ntp_msg *buf);

	/**
 * used by libnts app to close and unlink the shm ring buffer.
 */
	int ntm_ntp_shm_close(ntm_ntp_shm_context_t shm_ctx);

	/**
 * used by nt-monitor to disconnect the shm-ring based connection with libnts app
 */
	int ntm_ntp_shm_ntm_close(ntm_ntp_shm_context_t shm_ctx);

	/**
 * used by libnts app or nt-monitor to free the memory of nts_shm_context
 */
	void ntm_ntp_shm_destroy(ntm_ntp_shm_context_t shm_ctx);

#ifdef __cplusplus
};
#endif

#endif /* NTM_NTP_SHM_H_ */
