#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ntm_ntp_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DISABLE);

ntm_ntp_shm_context_t ntm_ntp_shm()
{
	ntm_ntp_shm_context_t shm_ctx;

	shm_ctx = (ntm_ntp_shm_context_t)
		malloc(sizeof(struct ntm_ntp_shm_context));
	shm_ctx->shm_stat = NTS_SHM_UNREADY;
	if (!shm_ctx)
	{
		DEBUG("[ntm_ntp_shm] create shm_ctx failed");
		return NULL;
	}

	return shm_ctx;
}

int ntm_ntp_shm_accept(
	ntm_ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen)
{
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);

	shm_ctx->ntm_ntpring_handle =
		ntm_ntp_shmring_init(shm_ctx->shm_addr, shm_ctx->addrlen);
	shm_ctx->shm_stat = NTS_SHM_READY;

	DEBUG("ntm_ntp_shm_accept pass");
	return 0;
}

int ntm_ntp_shm_connect(
	ntm_ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen)
{
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntm_ntpring_handle =
		ntm_ntp_get_shmring(shm_ctx->shm_addr, shm_ctx->addrlen);
	shm_ctx->shm_stat = NTS_SHM_READY;

	DEBUG("ntm_ntp_shm_connect pass");
	return 0;
}

int ntm_ntp_shm_send(
	ntm_ntp_shm_context_t shm_ctx, ntm_ntp_msg *buf)
{
	assert(shm_ctx);

	bool ret;
	ret = ntm_ntp_shmring_push(shm_ctx->ntm_ntpring_handle, buf);

	DEBUG("ntm_ntp_shm_send pass");
	return ret ? 0 : -1;
}

int ntm_ntp_shm_recv(
	ntm_ntp_shm_context_t shm_ctx, ntm_ntp_msg *buf)
{
	assert(shm_ctx);

	bool ret;
	ret = ntm_ntp_shmring_pop(shm_ctx->ntm_ntpring_handle, buf);

	return ret ? 0 : -1;
}

int ntm_ntp_shm_close(ntm_ntp_shm_context_t shm_ctx)
{
	assert(shm_ctx);

	ntm_ntp_shmring_free(shm_ctx->ntm_ntpring_handle, 1);
	shm_ctx->shm_stat = NTS_SHM_UNLINK;

	DEBUG("ntm_ntp_shm_close pass");
	return 0;
}

int ntm_ntp_shm_ntm_close(ntm_ntp_shm_context_t shm_ctx)
{
	assert(shm_ctx);

	ntm_ntp_shmring_free(shm_ctx->ntm_ntpring_handle, 0);
	shm_ctx->shm_stat = NTS_SHM_CLOSE;

	DEBUG("ntm_ntp_shm_ntm_close pass");
	return 0;
}

void ntm_ntp_shm_destroy(ntm_ntp_shm_context_t shm_ctx)
{
	assert(shm_ctx);

	free(shm_ctx->shm_addr);
	free(shm_ctx);
	shm_ctx = NULL;

	DEBUG("ntm_ntp_shm_destroy pass");
}
