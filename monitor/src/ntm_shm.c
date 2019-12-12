#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ntm_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

ntm_shm_context_t ntm_shm(){
	ntm_shm_context_t shm_ctx;
	
	shm_ctx = (ntm_shm_context_t) malloc(sizeof(struct ntm_shm_context));	
	shm_ctx->shm_stat = NTM_SHM_UNREADY;
	if(shm_ctx) {
		DEBUG("create shm_ctx pass");
	}

	return shm_ctx;
}


int ntm_shm_accept(ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);
	if(shm_ctx) {
		DEBUG("get shm_ctx pass ");
	}

	shm_ctx->shm_addr = (char *) malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ns_handle = ntm_shmring_init();  
	shm_ctx->shm_stat = NTM_SHM_READY;

	DEBUG("ntm_shm_accept pass");
	return 0;
}

int ntm_shm_connect(ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *) malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ns_handle = ntm_get_shmring();  
	shm_ctx->shm_stat = NTM_SHM_READY;

	DEBUG("ntm_shm_connect pass");
	return 0;
}

int ntm_shm_send(ntm_shm_context_t shm_ctx, char *buf, size_t len) {
	assert(shm_ctx);
	assert(len > 0);

	ntm_shmring_push(shm_ctx->ns_handle, buf, len);

	DEBUG("ntm_shm_send pass");
	return 0;
}

int ntm_shm_recv(ntm_shm_context_t shm_ctx, char *buf, size_t len) {
	assert(shm_ctx);
	assert(len > 0);

	int recv_size = ntm_shmring_pop(shm_ctx->ns_handle, buf, len);	

	DEBUG("ntm_shm_recv pass");
	return recv_size;
}

int ntm_shm_close(ntm_shm_context_t shm_ctx) {
	assert(shm_ctx);

	ntm_shmring_free(shm_ctx->ns_handle, 1);
	shm_ctx->shm_stat = NTM_SHM_UNLINK;
	free(shm_ctx->shm_addr);

	DEBUG("ntm_shm_close pass ");
	return 0;
}

int ntm_shm_nts_close(ntm_shm_context_t shm_ctx) {
	assert(shm_ctx);

	ntm_shmring_free(shm_ctx->ns_handle, 0);
	shm_ctx->shm_stat = NTM_SHM_CLOSE;
	free(shm_ctx->shm_addr);

	DEBUG("ntm_shm_nts_close pass ");
	return 0;
}

void ntm_shm_destroy(ntm_shm_context_t shm_ctx) {
	assert(shm_ctx);

	free(shm_ctx);
	DEBUG("ntm_shm_destroy pass ");
	return;
}
