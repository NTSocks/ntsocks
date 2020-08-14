/*
 * <p>Title: ntp2nts_shm.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTP2NTS_SHM_H_
#define NTP2NTS_SHM_H_

#include <stdio.h>

#include "shmring.h"
#include "ntp2nts_msg.h"
#include "shm_mempool.h"

#define RETRY_TIMES 5


#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum ntp_shm_stat
	{
		NTP_SHM_UNREADY = 0,
		NTP_SHM_READY,
		NTP_SHM_CLOSE,
		NTP_SHM_UNLINK
	} ntp_shm_stat;

	struct ntp_shm_context
	{
		ntp_shm_stat shm_stat;
		shmring_handle_t ntsring_handle;
		char *shm_addr;
		size_t addrlen;

        // for shm mempool
        shm_mp_handler * mp_handler;

	};

	typedef struct ntp_shm_context *ntp_shm_context_t;

	/**
 	 * used by libnts app or nt-monitor to create ntp_shm_context
     */
	ntp_shm_context_t ntp_shm();

	/**
     * used by nts shm server (consumer)
     */
	int ntp_shm_accept(ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
     * used by nt-monitor client (producer)
     */
	int ntp_shm_connect(ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
     * used by nt-monitor to send message to libnts app
     */
	int ntp_shm_send(ntp_shm_context_t shm_ctx, ntp_msg *buf);

	/**
     * used by libnts app to receive message from nt-monitor
     */
	ntp_msg * ntp_shm_recv(ntp_shm_context_t shm_ctx);

	/**
	 * used by libnts to front the top or next-pop ntp_msg
	 */
	ntp_msg * ntp_shm_front(ntp_shm_context_t shm_ctx);

	/**
     * used by libnts app to close and unlink the shm ring buffer.
     */
	int ntp_shm_close(ntp_shm_context_t shm_ctx);

	/**
     * used by nt-monitor to disconnect the shm-ring based connection with libnts app
     */
	int ntp_shm_nts_close(ntp_shm_context_t shm_ctx);

	/**
     * used by libnts app or nt-monitor to free the memory of ntp_shm_context
     */
	void ntp_shm_destroy(ntp_shm_context_t shm_ctx);

#ifdef __cplusplus
};
#endif

#endif /* NTP2NTS_SHM_H_ */
