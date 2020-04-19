/*
 * <p>Title: ntp_ntm_shm.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DISABLE);

ntp_ntm_shm_context_t ntp_ntm_shm() {
	ntp_ntm_shm_context_t shm_ctx;

	shm_ctx = (ntp_ntm_shm_context_t) malloc(sizeof(struct ntp_ntm_shm_context));
	shm_ctx->shm_stat = NTS_SHM_UNREADY;
	if (shm_ctx) {
		DEBUG("[ntp_ntm_shm] create shm_ctx pass");
	}

	return shm_ctx;
}

int ntp_ntm_shm_accept(ntp_ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntp_ntmring_handle = ntp_ntm_shmring_init(shm_ctx->shm_addr, shm_ctx->addrlen);
	shm_ctx->shm_stat = NTS_SHM_READY;

	DEBUG("ntp_ntm_shm_accept pass");
	return 0;
}


int ntp_ntm_shm_connect(ntp_ntm_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntp_ntmring_handle = ntp_ntm_get_shmring(shm_ctx->shm_addr, shm_ctx->addrlen); /// note: need to improve
	shm_ctx->shm_stat = NTS_SHM_READY;

	DEBUG("ntp_ntm_shm_connect pass");
	return 0;
}


int ntp_ntm_shm_send(ntp_ntm_shm_context_t shm_ctx, ntp_ntm_msg *buf) {
	assert(shm_ctx);

	bool ret;
	ret = ntp_ntm_shmring_push(shm_ctx->ntp_ntmring_handle, buf);

	DEBUG("ntp_ntm_shm_send pass");
	return ret ? 0 : -1;
}


int ntp_ntm_shm_recv(ntp_ntm_shm_context_t shm_ctx, ntp_ntm_msg *buf) {
	assert(shm_ctx);

	bool ret;
	ret = ntp_ntm_shmring_pop(shm_ctx->ntp_ntmring_handle, buf);

	DEBUG("ntp_ntm_shm_recv pass");
	return ret ? 0 : -1;
}


int ntp_ntm_shm_close(ntp_ntm_shm_context_t shm_ctx) {
	assert(shm_ctx);

	ntp_ntm_shmring_free(shm_ctx->ntp_ntmring_handle, 1);
	shm_ctx->shm_stat = NTS_SHM_UNLINK;
	free(shm_ctx->ntp_ntmring_handle);

	DEBUG("ntp_ntm_shm_close pass");
	return 0;
}


int ntp_ntm_shm_ntm_close(ntp_ntm_shm_context_t shm_ctx) {
	assert(shm_ctx);

	ntp_ntm_shmring_free(shm_ctx->ntp_ntmring_handle, 0);
	shm_ctx->shm_stat = NTS_SHM_CLOSE;
	free(shm_ctx->ntp_ntmring_handle);

	DEBUG("ntp_ntm_shm_ntm_close pass");
	return 0;
}


void ntp_ntm_shm_destroy(ntp_ntm_shm_context_t shm_ctx) {
	assert(shm_ctx);

	free(shm_ctx->shm_addr);
	free(shm_ctx);
	shm_ctx = NULL;
	DEBUG("ntp_ntm_shm_destroy pass");
}

