#include <assert.h>

#include "epoll_sem_shm.h"
#include "utils.h"
#include "nt_log.h"

#ifdef ENABLE_DEBUG
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);
#else
DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);
#endif //ENABLE_DEBUG

epoll_sem_shm_ctx_t epoll_sem_shm()
{
    epoll_sem_shm_ctx_t ctx;

    ctx = (epoll_sem_shm_ctx_t)
        calloc(1, sizeof(struct _epoll_sem_shm_ctx));
    if (UNLIKELY(!ctx))
    {
        ERR("malloc epoll_sem_shm_ctx_t failed.");
        return NULL;
    }

    ctx->stat = SEM_SHMRING_UNREADY;
    return ctx;
}

int epoll_sem_shm_accept(
    epoll_sem_shm_ctx_t shm_ctx, char *shm_addr, size_t addrlen)
{
    assert(shm_ctx);
    assert(addrlen > 0);
    assert(shm_addr);

    shm_ctx->shm_addr = (char *)malloc(addrlen);
    if (UNLIKELY(!shm_ctx->shm_addr))
        return -1;

    memset(shm_ctx->shm_addr, 0, addrlen);
    shm_ctx->addrlen = addrlen;
    memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
    shm_ctx->handle =
        sem_shmring_create(shm_ctx->shm_addr,
                           shm_ctx->addrlen, EPOLL_MSG_SIZE, DEFAULT_MAX_NUM_MSG);
    if (UNLIKELY(!shm_ctx->handle))
    {
        ERR("epoll_sem_shm_accept() failed.");
        free(shm_ctx->shm_addr);
        return -1;
    }

    shm_ctx->stat = SEM_SHMRING_READY;
    DEBUG("epoll_sem_shm_accept() success.");

    return 0;
}

int epoll_sem_shm_connect(
    epoll_sem_shm_ctx_t shm_ctx, char *shm_addr, size_t addrlen)
{
    assert(shm_ctx);
    assert(addrlen > 0);
    assert(shm_addr);

    shm_ctx->shm_addr = (char *)malloc(addrlen);
    if (UNLIKELY(!shm_ctx->shm_addr))
    {
        return -1;
    }

    memset(shm_ctx->shm_addr, 0, addrlen);
    shm_ctx->addrlen = addrlen;
    memcpy(shm_ctx->shm_addr, shm_addr, addrlen);

    shm_ctx->handle =
        sem_shmring_init(shm_ctx->shm_addr,
                         shm_ctx->addrlen, EPOLL_MSG_SIZE, DEFAULT_MAX_NUM_MSG);
    if (UNLIKELY(!shm_ctx->handle))
    {
        ERR("epoll_sem_shm_accept() failed.");
        free(shm_ctx->shm_addr);
        return -1;
    }

    shm_ctx->stat = SEM_SHMRING_READY;
    DEBUG("epoll_sem_shm_accept() success.");

    return 0;
}

int epoll_sem_shm_send(
    epoll_sem_shm_ctx_t shm_ctx, epoll_msg *msg)
{
    assert(shm_ctx);
    assert(msg);

    int rc;
    rc = sem_shmring_push(
        shm_ctx->handle, (char *)msg, EPOLL_MSG_SIZE);
    if (UNLIKELY(rc == -1))
    {
        ERR("epoll_sem_shm_send() failed and maybe sem_shmring is full.");
        return -1;
    }

    return 0;
}

int epoll_sem_shm_recv(
    epoll_sem_shm_ctx_t shm_ctx, epoll_msg *recv_msg)
{
    assert(shm_ctx);
    assert(recv_msg);

    int rc;
    rc = sem_shmring_pop(
        shm_ctx->handle, (char *)recv_msg, EPOLL_MSG_SIZE);
    if (UNLIKELY(rc == -1))
    {
        ERR("epoll_sem_shm_recv() failed and maybe sem_shmring is empty.");
        return -1;
    }

    return 0;
}

int epoll_sem_try_exit(epoll_sem_shm_ctx_t shm_ctx)
{
    assert(shm_ctx);

    epoll_msg msg;
    msg.id = 1;
    msg.msg_type = EPOLL_MSG_CLOSE;
    int rc;
    rc = sem_shmring_push(
        shm_ctx->handle, (char *)&msg, EPOLL_MSG_SIZE);
    if (UNLIKELY(rc == -1))
    {
        ERR("epoll_sem_shm_send() failed and maybe sem_shmring is full.");
        return -1;
    }

    return 0;
}

int epoll_sem_shm_close(
    epoll_sem_shm_ctx_t shm_ctx, bool is_slave)
{
    assert(shm_ctx);

    if (shm_ctx->handle)
    {
        sem_shmring_free(shm_ctx->handle, !is_slave);
        shm_ctx->handle = NULL;
    }

    if (shm_ctx->shm_addr)
    {
        free(shm_ctx->shm_addr);
        shm_ctx->shm_addr = NULL;
    }

    shm_ctx->stat = is_slave ? SEM_SHMRING_CLOSE : SEM_SHMRING_UNLINK;
    DEBUG("epoll_sem_shm_close() success");
    return 0;
}

int epoll_sem_shm_master_close(epoll_sem_shm_ctx_t shm_ctx)
{
    return epoll_sem_shm_close(shm_ctx, false);
}

int epoll_sem_shm_slave_close(epoll_sem_shm_ctx_t shm_ctx)
{
    return epoll_sem_shm_close(shm_ctx, true);
}

void epoll_sem_shm_destroy(epoll_sem_shm_ctx_t shm_ctx)
{
    assert(shm_ctx);

    free(shm_ctx);
    shm_ctx = NULL;
    DEBUG("epoll_sem_shm_destroy() success");
}