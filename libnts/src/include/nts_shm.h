/*
 * <p>Title: nts_shm.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTS_SHM_H_
#define NTS_SHM_H_

#include <stdio.h>

#include "nts_shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum nts_shm_stat {
	NTS_SHM_UNREADY = 0,
	NTS_SHM_READY,
	NTS_SHM_CLOSE,
	NTS_SHM_UNLINK
} nts_shm_stat;

struct nts_shm_context {
	nts_shm_stat shm_stat;
	nts_shmring_handle_t ntsring_handle;
	char *shm_addr;
	size_t addrlen;
};

typedef struct nts_shm_context* nts_shm_context_t;

/**
 * used by libnts app or nt-monitor to create nts_shm_context
 */
nts_shm_context_t nts_shm();

/**
 * used by nts shm server (consumer)
 */
int nts_shm_accept(nts_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

/**
 * used by nt-monitor client (producer)
 */
int nts_shm_connect(nts_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

/**
 * used by nt-monitor to send message to libnts app
 */
int nts_shm_send(nts_shm_context_t shm_ctx, nts_msg *buf);

/**
 * used by libnts app to receive message from nt-monitor
 */
int nts_shm_recv(nts_shm_context_t shm_ctx, nts_msg *buf);

/**
 * used by libnts app to close and unlink the shm ring buffer.
 */
int nts_shm_close(nts_shm_context_t shm_ctx);

/**
 * used by nt-monitor to disconnect the shm-ring based connection with libnts app
 */
int nts_shm_ntm_close(nts_shm_context_t shm_ctx);

/**
 * used by libnts app or nt-monitor to free the memory of nts_shm_context
 */
void nts_shm_destroy(nts_shm_context_t shm_ctx);

#ifdef __cplusplus
};
#endif

#endif /* NTS_SHM_H_ */
