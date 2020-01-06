/*
 * <p>Title: nts_shm.c</p>
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

#include "ntp_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

nts_shm_context_t nts_shm() {
	nts_shm_context_t shm_ctx;

	shm_ctx = (nts_shm_context_t) malloc(sizeof(struct nts_shm_context));
	shm_ctx->shm_stat = NTS_SHM_UNREADY;
	if (shm_ctx) {
		DEBUG("[nts_shm] create shm_ctx pass");
	}

	return shm_ctx;
}


int nts_shm_accept(nts_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntsring_handle = nts_shmring_init(shm_ctx->shm_addr, shm_ctx->addrlen);
	shm_ctx->shm_stat = NTS_SHM_READY;

	DEBUG("nts_shm_accept pass");
	return 0;
}


int nts_shm_connect(nts_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntsring_handle = nts_get_shmring(shm_ctx->shm_addr, shm_ctx->addrlen); /// note: need to improve
	shm_ctx->shm_stat = NTS_SHM_READY;

	DEBUG("nts_shm_connect pass");
	return 0;
}


int nts_shm_send(nts_shm_context_t shm_ctx, ntp_msg *buf) {
	assert(shm_ctx);

	bool ret;
	ret = nts_shmring_push(shm_ctx->ntsring_handle, buf);

	DEBUG("nts_shm_send pass");
	return ret ? 0 : -1;
}


int nts_shm_recv(nts_shm_context_t shm_ctx, ntp_msg *buf) {
	assert(shm_ctx);

	bool ret;
	ret = nts_shmring_pop(shm_ctx->ntsring_handle, buf);

	DEBUG("nts_shm_recv pass");
	return ret ? 0 : -1;
}

//创建者销毁
int nts_shm_close(nts_shm_context_t shm_ctx) {
	assert(shm_ctx);

	nts_shmring_free(shm_ctx->ntsring_handle, 1);
	shm_ctx->shm_stat = NTS_SHM_UNLINK;
	free(shm_ctx->shm_addr);
	shm_ctx->shm_addr = NULL;

	DEBUG("nts_shm_close pass");
	return 0;
}

//使用者销毁
int nts_shm_ntm_close(nts_shm_context_t shm_ctx) {
	assert(shm_ctx);

	nts_shmring_free(shm_ctx->ntsring_handle, 0);
	shm_ctx->shm_stat = NTS_SHM_CLOSE;
	free(shm_ctx->shm_addr);
	shm_ctx->shm_addr = NULL;

	DEBUG("nts_shm_ntm_close pass");
	return 0;
}


void nts_shm_destroy(nts_shm_context_t shm_ctx) {
	assert(shm_ctx);

	free(shm_ctx);
	shm_ctx = NULL;
	DEBUG("nts_shm_destroy pass");
}

