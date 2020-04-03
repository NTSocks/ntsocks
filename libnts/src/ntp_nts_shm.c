/*
 * <p>Title: ntp_nts_shm.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang, Jing7
 * @date Nov 23, 2019 
 * @version 1.0
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h>

#include "ntp_nts_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);

ntp_shm_context_t ntp_shm() {
	ntp_shm_context_t shm_ctx;

	shm_ctx = (ntp_shm_context_t) malloc(sizeof(struct ntp_shm_context));
	shm_ctx->shm_stat = NTP_SHM_UNREADY;
	if (shm_ctx) {
		DEBUG("[ntp_shm] create shm_ctx pass");
	}

	return shm_ctx;
}

int ntp_shm_accept(ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntsring_handle = ntp_shmring_init(shm_ctx->shm_addr, shm_ctx->addrlen);
	shm_ctx->shm_stat = NTP_SHM_READY;

	DEBUG("ntp_shm_accept pass");
	return 0;
}


int ntp_shm_connect(ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntsring_handle = ntp_get_shmring(shm_ctx->shm_addr, shm_ctx->addrlen); /// note: need to improve
	shm_ctx->shm_stat = NTP_SHM_READY;

	DEBUG("ntp_shm_connect pass");
	return 0;
}


int ntp_shm_send(ntp_shm_context_t shm_ctx, ntp_msg *buf) {
	assert(shm_ctx);

	bool ret;
	DEBUG("ntp_shmring_push invoked");
	ret = ntp_shmring_push(shm_ctx->ntsring_handle, buf);
	//TODO: add the timeout limit for retry push times.
	while(!ret) {
		sched_yield();
		ret = ntp_shmring_push(shm_ctx->ntsring_handle, buf);
	}

	DEBUG("ntp_shm_send pass with ret=%d", ret);
	return ret ? 0 : -1;
}


int ntp_shm_recv(ntp_shm_context_t shm_ctx, ntp_msg *buf) {
	assert(shm_ctx);

	bool ret;
	ret = ntp_shmring_pop(shm_ctx->ntsring_handle, buf);
	//TODO: add the timeout limit for retry pop times.
	while(!ret) {
		sched_yield();
		ret = ntp_shmring_pop(shm_ctx->ntsring_handle, buf);
	}

	DEBUG("ntp_shm_recv pass");
	return ret ? 0 : -1;
}

int ntp_shm_front(ntp_shm_context_t shm_ctx, ntp_msg *buf) {
	assert(shm_ctx);

	bool ret;
	ret = ntp_shmring_front(shm_ctx->ntsring_handle, buf);

	// DEBUG("ntp_shm_recv pass");
	return ret ? 0 : -1;
}

//创建者销毁
int ntp_shm_close(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

	ntp_shmring_free(shm_ctx->ntsring_handle, 1);
	shm_ctx->shm_stat = NTP_SHM_UNLINK;
	free(shm_ctx->shm_addr);
	shm_ctx->shm_addr = NULL;

	DEBUG("ntp_shm_close pass");
	return 0;
}

//使用者销毁
int ntp_shm_nts_close(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

	ntp_shmring_free(shm_ctx->ntsring_handle, 0);
	shm_ctx->shm_stat = NTP_SHM_CLOSE;
	free(shm_ctx->shm_addr);
	shm_ctx->shm_addr = NULL;

	DEBUG("ntp_shm_ntm_close pass");
	return 0;
}


void ntp_shm_destroy(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

	free(shm_ctx);
	shm_ctx = NULL;
	DEBUG("ntp_shm_destroy pass");
}

