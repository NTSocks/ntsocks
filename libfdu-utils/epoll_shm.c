#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h>

#include "epoll_shm.h"
#include "utils.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

epoll_shm_context_t epoll_shm()
{
    epoll_shm_context_t ctx;

    ctx = (epoll_shm_context_t)malloc(sizeof(struct _epoll_shm_context));
    if (ctx)
    {
        DEBUG("epoll_shm() pass");
        ctx->stat = SHM_UNREADY;
        return ctx;
    }

    return NULL;
}

int epoll_shm_accept(epoll_shm_context_t shm_ctx, char *shm_addr, size_t addrlen)
{
    assert(shm_ctx);
    assert(addrlen > 0);
    assert(shm_addr);

    shm_ctx->shm_addr = (char *)malloc(addrlen);
    if (UNLIKELY(shm_ctx->shm_addr == NULL))
        return -1;

    memset(shm_ctx->shm_addr, 0, addrlen);
    shm_ctx->addrlen = addrlen;
    memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
    shm_ctx->handle = shmring_create(shm_ctx->shm_addr,
                                     shm_ctx->addrlen, EPOLL_MSG_SIZE, DEFAULT_MAX_NUM_MSG);
    if (!shm_ctx->handle)
    {
        ERR("epoll_shm_accept() failed.");
        free(shm_ctx->shm_addr);
        return -1;
    }

    shm_ctx->stat = SHM_READY;
    DEBUG("epoll_shm_accept() pass.");

    return 0;
}

int epoll_shm_connect(epoll_shm_context_t shm_ctx, char *shm_addr, size_t addrlen)
{
    assert(shm_ctx);
    assert(addrlen > 0);

    shm_ctx->shm_addr = (char *)malloc(addrlen);
    if (shm_ctx->shm_addr == NULL)
        return -1;

    memset(shm_ctx->shm_addr, 0, addrlen);
    shm_ctx->addrlen = addrlen;
    memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
    shm_ctx->handle = shmring_init(shm_ctx->shm_addr, shm_ctx->addrlen, EPOLL_MSG_SIZE, DEFAULT_MAX_NUM_MSG);
    if (!shm_ctx->handle)
    {
        ERR("epoll_shm_connect() failed.");
        free(shm_ctx->shm_addr);
        return -1;
    }

    shm_ctx->stat = SHM_READY;
    DEBUG("epoll_shm_connect() pass.");

    return 0;
}

int epoll_shm_send(epoll_shm_context_t shm_ctx, epoll_msg *msg)
{
    assert(shm_ctx);
    assert(msg);

    bool retval;
    retval = shmring_push(shm_ctx->handle, (char *)msg, EPOLL_MSG_SIZE);
    while (!retval)
    {
        sched_yield();
        INFO("shmring_push() failed and maybe shmring is full.");
        retval = shmring_push(shm_ctx->handle, (char *)msg, EPOLL_MSG_SIZE);
    }

    DEBUG("epoll_shm_send() pass with ret=%d", retval);
    return retval ? 0 : -1;
}

int epoll_shm_recv(epoll_shm_context_t shm_ctx, epoll_msg *recv_msg)
{
    assert(shm_ctx);
    assert(recv_msg);

    bool retval;
    retval = shmring_pop(shm_ctx->handle, (char *)recv_msg, EPOLL_MSG_SIZE);
    int retry_cnt = 1;
    while (!retval)
    {
        if (retry_cnt > EPOLL_RETRY_TIMES)
        {
            INFO("epoll_shm_recv() failed: the epoll_shm_recv retry times > RETRY_TIME(%d)", EPOLL_RETRY_TIMES);
            return -1;
        }

        sched_yield();
        retval = shmring_pop(shm_ctx->handle, (char *)recv_msg, EPOLL_MSG_SIZE);
        retry_cnt++;
    }

    return 0;
}

int epoll_shm_close(epoll_shm_context_t shm_ctx, bool is_slave)
{
    assert(shm_ctx);

    if (shm_ctx->handle)
    {
        shmring_free(shm_ctx->handle, !is_slave);
        shm_ctx->handle = NULL;
    }

    if (shm_ctx->shm_addr)
    {
        free(shm_ctx->shm_addr);
        shm_ctx->shm_addr = NULL;
    }

    shm_ctx->stat = is_slave ? SHM_CLOSE : SHM_UNLINK;
    DEBUG("epoll_shm_close() pass");
    return 0;
}
int epoll_shm_master_close(epoll_shm_context_t shm_ctx)
{
    return epoll_shm_close(shm_ctx, false);
}
int epoll_shm_slave_close(epoll_shm_context_t shm_ctx)
{
    return epoll_shm_close(shm_ctx, true);
}

void epoll_shm_destroy(epoll_shm_context_t shm_ctx)
{
    assert(shm_ctx);

    free(shm_ctx);
    shm_ctx = NULL;
    DEBUG("epoll_shm_destroy() pass");
}