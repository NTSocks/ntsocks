/*
 * <p>Title: nt_backlog.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Jan 5, 2020 
 * @version 1.0
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "nt_backlog.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

/**
 *
 * @param listener
 * @param shm_addr
 * @param addrlen
 * @param backlog_size
 * @param is_owner : if ntm, is_owner is true; else, is_owner is false
 * @return
 */
static inline nt_backlog_context_t _backlog_init(nt_listener_t listener, char *shm_addr,
                                                 size_t addrlen, int backlog_size, bool is_owner) {
    assert(listener);
    assert(shm_addr);
    assert(addrlen > 0);

    nt_backlog_context_t backlog_ctx;

    backlog_ctx = calloc(1, sizeof(struct nt_backlog_context));
    backlog_ctx->backlog_stat = BACKLOG_UNREADY;
    if(!backlog_ctx) {
        DEBUG("[nt_backlog] create backlog context failed");
        return NULL;
    }

    backlog_ctx->backlog_size = (backlog_size <= 0 ? BACKLOG_SIZE : backlog_size);
    backlog_ctx->listener = listener;

    backlog_ctx->shm_addr = malloc(addrlen);
    memset(backlog_ctx->shm_addr, 0, addrlen);
    backlog_ctx->addrlen = addrlen;
    memcpy(backlog_ctx->shm_addr, shm_addr, addrlen);
    if(is_owner) {
        backlog_ctx->shmring_handle = nt_spsc_shmring_init(backlog_ctx->shm_addr, addrlen, backlog_ctx->backlog_size, NT_SOCK_SIZE);
    } else {
        backlog_ctx->shmring_handle = nt_get_spsc_shmring(backlog_ctx->shm_addr, addrlen, backlog_ctx->backlog_size, NT_SOCK_SIZE);
    }

    if (backlog_ctx->shmring_handle) {
        backlog_ctx->backlog_stat = BACKLOG_READY;
        DEBUG("backlog_nts init pass");
        return backlog_ctx;
    }

    return NULL;

}


nt_backlog_context_t backlog_nts(nt_listener_t listener,
                                 char *shm_addr, size_t addrlen, int backlog_size) {
    return _backlog_init(listener, shm_addr, addrlen, backlog_size, false);
}


nt_backlog_context_t backlog_ntm(nt_listener_t listener,
                                 char *shm_addr, size_t addrlen, int backlog_size) {

    return _backlog_init(listener, shm_addr, addrlen, backlog_size, true);
}


int backlog_push(nt_backlog_context_t backlog_ctx, nt_socket_t socket) {
    assert(backlog_ctx);
    assert(socket);
    assert(backlog_ctx->backlog_stat == BACKLOG_READY);

    bool ret;
    ret = nt_spsc_shmring_push(backlog_ctx->shmring_handle, (char *) socket, NT_SOCK_SIZE);

    DEBUG("backlog push pass");
    return ret ? 0 : -1;
}


int backlog_pop(nt_backlog_context_t backlog_ctx, nt_socket_t socket) {
    assert(backlog_ctx);
    assert(socket);
    assert(backlog_ctx->backlog_stat == BACKLOG_READY);

    bool ret;
    ret = nt_spsc_shmring_pop(backlog_ctx->shmring_handle, (char *) socket, NT_SOCK_SIZE);

    DEBUG("backlog pop pass");
    return ret ? 0 : -1;
}

static inline int _backlog_close(nt_backlog_context_t backlog_ctx, bool is_owner) {
    assert(backlog_ctx);
    assert(backlog_ctx->backlog_stat == BACKLOG_READY);

    if (is_owner) {
        nt_spsc_shmring_free(backlog_ctx->shmring_handle, 1);
        backlog_ctx->backlog_stat = BACKLOG_UNLINK;
    } else{
        nt_spsc_shmring_free(backlog_ctx->shmring_handle, 0);
        backlog_ctx->backlog_stat = BACKLOG_CLOSE;
    }

    free(backlog_ctx->shm_addr);
    backlog_ctx->shm_addr = NULL;

    DEBUG("backlog close pass");
    return 0;
}

int backlog_nts_close(nt_backlog_context_t backlog_ctx) {

    return _backlog_close(backlog_ctx, false);
}


int backlog_ntm_close(nt_backlog_context_t backlog_ctx) {

    return _backlog_close(backlog_ctx, true);
}

void backlog_destroy(nt_backlog_context_t backlog_ctx) {
    assert(backlog_ctx);

    free(backlog_ctx);
    backlog_ctx = NULL;
    DEBUG("nt backlog destroy pass");
}


bool backlog_is_full(nt_backlog_context_t backlog_ctx) {
    assert(backlog_ctx);
    
    return nt_spsc_shmring_is_full(backlog_ctx->shmring_handle);
}