/*
 * <p>Title: ntp_ntm_shm.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTP_NTM_SHM_H_
#define NTP_NTM_SHM_H_

#include <stdio.h>

#include "ntp_ntm_shmring.h"

#ifdef __cplusplus
extern "C"
{
#endif

	struct ntp_ntm_shm_context
	{
		ntm_shm_stat shm_stat;
		ntp_ntm_shmring_handle_t ntp_ntmring_handle;
		char *shm_addr;
		size_t addrlen;
	};

	typedef struct ntp_ntm_shm_context *ntp_ntm_shm_context_t;

	/**
	 * used by libnts app or nt-monitor to create nts_shm_context
	 */
	ntp_ntm_shm_context_t ntp_ntm_shm(void);

	/**
	 * used by ntp_ntm shm server (consumer)
	 */
	int ntp_ntm_shm_accept(ntp_ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
	 * used by nt-monitor client (producer)
	 */
	int ntp_ntm_shm_connect(ntp_ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
	 * used by nt-monitor to send message to libnts app
	 */
	int ntp_ntm_shm_send(ntp_ntm_shm_context_t shm_ctx, ntp_ntm_msg *buf);

	/**
	 * used by libnts app to receive message from nt-monitor
	 */
	int ntp_ntm_shm_recv(ntp_ntm_shm_context_t shm_ctx, ntp_ntm_msg *buf);

	/**
	 * used by libnts app to close and unlink the shm ring buffer.
	 */
	int ntp_ntm_shm_close(ntp_ntm_shm_context_t shm_ctx);

	/**
	 * used by nt-monitor to disconnect the shm-ring based connection with libnts app
	 */
	int ntp_ntm_shm_ntm_close(ntp_ntm_shm_context_t shm_ctx);

	/**
	 * used by libnts app or nt-monitor to free the memory of nts_shm_context
	 */
	void ntp_ntm_shm_destroy(ntp_ntm_shm_context_t shm_ctx);

#ifdef __cplusplus
};
#endif

#endif /* NTP_NTM_SHM_H_ */
