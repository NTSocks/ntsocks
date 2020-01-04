#ifndef NTM_SHM_H
#define NTM_SHM_H

#include <stdio.h>

#include "ntm_shmring.h"

typedef enum ntm_shm_stat {
	NTM_SHM_UNREADY = 0,
	NTM_SHM_READY,
	NTM_SHM_CLOSE,
	NTM_SHM_UNLINK
} ntm_shm_stat;

struct ntm_shm_context {
	ntm_shm_stat shm_stat;	
	ntm_shmring_handle_t ns_handle;
	char *shm_addr;
	size_t addrlen;
};

typedef struct ntm_shm_context* ntm_shm_context_t;

/* init ntm_shm channel */
ntm_shm_context_t ntm_shm();

int ntm_shm_accept(ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

int ntm_shm_connect(ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

int ntm_shm_send(ntm_shm_context_t shm_ctx, char *buf, size_t len);

int ntm_shm_recv(ntm_shm_context_t shm_ctx, char *buf, size_t len);

int ntm_shm_close(ntm_shm_context_t shm_ctx);

int ntm_shm_nts_close(ntm_shm_context_t shm_ctx);

void ntm_shm_destroy(ntm_shm_context_t shm_ctx);

#endif // NTM_SHM_H
