#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h>

#include "ntp2nts_shm.h"
#include "utils.h"
#include "nt_log.h"

#ifdef ENABLE_DEBUG
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);
#else
DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);
#endif //ENABLE_DEBUG

ntp_shm_context_t ntp_shm(size_t slot_size)
{
    ntp_shm_context_t shm_ctx;

    shm_ctx = (ntp_shm_context_t)
        malloc(sizeof(struct ntp_shm_context));
    if (!shm_ctx)
    {
        ERR("create ntp_shm_context_t failed");
        return NULL;
    }

    shm_ctx->shm_stat = NTP_SHM_UNREADY;
    shm_ctx->slot_size = slot_size;

    DEBUG("[ntp_shm] create shm_ctx success");
    return shm_ctx;
}

int ntp_shm_accept(
    ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen)
{
    assert(shm_ctx);
    assert(addrlen > 0);

    memset(shm_ctx->shm_addr, 0, NTP_SHMADDR_SIZE);
    shm_ctx->addrlen = addrlen;
    memcpy(shm_ctx->shm_addr, shm_addr, addrlen);

    shm_ctx->ntsring_handle = shmring_create(
        shm_ctx->shm_addr, shm_ctx->addrlen,
        NTP_MSG_IDX_SIZE, DEFAULT_MAX_NUM_NTP_MSG);
    if (!shm_ctx->ntsring_handle)
    {
        ERR("init shmring failed.");
        free(shm_ctx->shm_addr);
        return -1;
    }

    // for bulk operations on shmring
    shm_ctx->node_idxs_cache = (char **)
        calloc(DEFAULT_MAX_BUFS, sizeof(char *));
    if (!shm_ctx->node_idxs_cache)
    {
        ERR("malloc shm_ctx->node_idxs_cache failed.");
        goto FAIL;
    }

    shm_ctx->node_idx_value_cache =
        (int *)calloc(DEFAULT_MAX_BUFS, sizeof(int));
    if (!shm_ctx->node_idx_value_cache)
    {
        ERR("malloc shm_ctx->node_idx_value_cache failed.");
        goto FAIL;
    }

    shm_ctx->max_lens =
        (size_t *)calloc(DEFAULT_MAX_BUFS, sizeof(size_t));
    if (!shm_ctx->max_lens)
    {
        ERR("malloc shm_ctx->max_lens failed.");
        goto FAIL;
    }

    for (int i = 0; i < DEFAULT_MAX_BUFS; i++)
    {
        shm_ctx->max_lens[i] = sizeof(int);
        shm_ctx->node_idxs_cache[i] =
            (char *)&(shm_ctx->node_idx_value_cache[i]);
    }

    /* init shm mempool */
    char mp_name[30] = {0};
    sprintf(mp_name, "%s-mp", shm_addr);
    shm_ctx->mp_handler = shm_mp_init(shm_ctx->slot_size,
                                      DEFAULT_MAX_NUM_NTP_MSG * 4, mp_name, strlen(mp_name));
    if (!shm_ctx->mp_handler)
    {
        ERR("init shm mempool failed.");
        goto FAIL;
    }
    shm_ctx->shm_stat = NTP_SHM_READY;

    DEBUG("ntp_shm_accept success");
    return 0;

FAIL:
    if (shm_ctx->mp_handler)
    {
        shm_mp_destroy(shm_ctx->mp_handler, 1);
        shm_ctx->mp_handler = NULL;
    }

    if (shm_ctx->node_idxs_cache)
    {
        free(shm_ctx->node_idxs_cache);
        shm_ctx->node_idxs_cache = NULL;
    }

    if (shm_ctx->node_idx_value_cache)
    {
        free(shm_ctx->node_idx_value_cache);
        shm_ctx->node_idx_value_cache = NULL;
    }

    if (shm_ctx->max_lens)
    {
        free(shm_ctx->max_lens);
        shm_ctx->max_lens = NULL;
    }

    if (shm_ctx->ntsring_handle)
    {
        shmring_free(shm_ctx->ntsring_handle, true);
        shm_ctx->ntsring_handle = NULL;
    }

    return -1;
}

int ntp_shm_connect(
    ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen)
{
    assert(shm_ctx);
    assert(addrlen > 0);

    memset(shm_ctx->shm_addr, 0, NTP_SHMADDR_SIZE);
    shm_ctx->addrlen = addrlen;
    memcpy(shm_ctx->shm_addr, shm_addr, addrlen);

    shm_ctx->ntsring_handle = shmring_init(shm_ctx->shm_addr,
                                           shm_ctx->addrlen, NTP_MSG_IDX_SIZE, DEFAULT_MAX_NUM_NTP_MSG);
    if (!shm_ctx->ntsring_handle)
    {
        ERR("get shmring failed.");
        free(shm_ctx->shm_addr);
        return -1;
    }

    // for bulk operations on shmring
    shm_ctx->node_idxs_cache =
        (char **)calloc(DEFAULT_MAX_BUFS, sizeof(char *));
    if (!shm_ctx->node_idxs_cache)
    {
        ERR("malloc shm_ctx->node_idxs_cache failed.");
        goto FAIL;
    }

    shm_ctx->node_idx_value_cache =
        (int *)calloc(DEFAULT_MAX_BUFS, sizeof(int));
    if (!shm_ctx->node_idx_value_cache)
    {
        ERR("malloc shm_ctx->node_idx_value_cache failed.");
        goto FAIL;
    }

    shm_ctx->max_lens =
        (size_t *)calloc(DEFAULT_MAX_BUFS, sizeof(size_t));
    if (!shm_ctx->max_lens)
    {
        ERR("malloc shm_ctx->max_lens failed.");
        goto FAIL;
    }

    for (int i = 0; i < DEFAULT_MAX_BUFS; i++)
    {
        shm_ctx->max_lens[i] = sizeof(int);
        shm_ctx->node_idxs_cache[i] =
            (char *)&(shm_ctx->node_idx_value_cache[i]);
    }

    /* init shm mempool */
    char mp_name[30] = {0};
    sprintf(mp_name, "%s-mp", shm_addr);
    shm_ctx->mp_handler = shm_mp_init(shm_ctx->slot_size,
                                      DEFAULT_MAX_NUM_NTP_MSG * 4, mp_name, strlen(mp_name));
    if (!shm_ctx->mp_handler)
    {
        ERR("init shm mempool failed. ");
        goto FAIL;
    }
    shm_ctx->shm_stat = NTP_SHM_READY;

    DEBUG("ntp_shm_connect success");
    return 0;

FAIL:

    if (shm_ctx->mp_handler)
    {
        shm_mp_destroy(shm_ctx->mp_handler, 0);
        shm_ctx->mp_handler = NULL;
    }

    if (shm_ctx->node_idxs_cache)
    {
        free(shm_ctx->node_idxs_cache);
        shm_ctx->node_idxs_cache = NULL;
    }

    if (shm_ctx->node_idx_value_cache)
    {
        free(shm_ctx->node_idx_value_cache);
        shm_ctx->node_idx_value_cache = NULL;
    }

    if (shm_ctx->max_lens)
    {
        free(shm_ctx->max_lens);
        shm_ctx->max_lens = NULL;
    }

    if (shm_ctx->ntsring_handle)
    {
        shmring_free(shm_ctx->ntsring_handle, false);
        shm_ctx->ntsring_handle = NULL;
    }

    return -1;
}

int ntp_shm_send(
    ntp_shm_context_t shm_ctx, ntp_msg *buf)
{
    assert(shm_ctx);
    assert(buf);

    shm_mempool_node *mp_node;
    mp_node = shm_mp_node_by_shmaddr(
        shm_ctx->mp_handler, (char *)buf);
    assert(mp_node);

    int ret;
    ret = shmring_push(shm_ctx->ntsring_handle,
                       (char *)&mp_node->node_idx, NODE_IDX_SIZE);
    //TODO: add the timeout limit for retry push times.
    // int retry_times = 0;
    while (ret != SUCCESS)
    {
        // once the retry_times is greater than pre-define 'RETRY_TIMES' (default 5),
        //  indicate current ntp_shm ring is empty and immediately return NULL
        // if(retry_times > RETRY_TIMES)
        //     break;
        INFO("shmring_push failed and maybe shmring is full.");
        ret = shmring_push(shm_ctx->ntsring_handle,
                           (char *)&mp_node->node_idx, NODE_IDX_SIZE);
        // retry_times ++;
    }

    DEBUG("ntp_shm_send pass with ret=%d", ret);
    return ret;
}

int ntp_shm_recv(
    ntp_shm_context_t shm_ctx, ntp_msg **buf)
{
    assert(shm_ctx);
    assert(buf);

    int ret;
    int node_idx = -1;
    ret = shmring_pop(
        shm_ctx->ntsring_handle, (char *)&node_idx, NODE_IDX_SIZE);
    //TODO: add the timeout limit for retry pop times.
    int retry_times = 0;
    while (ret != SUCCESS)
    {
        // once the retry_times is greater than
        //  pre-define 'RETRY_TIMES' (default 5),
        //  indicate current ntp_shm ring is
        //  empty and immediately return NULL
        if (retry_times > RETRY_TIMES)
        {
            return FAILED;
        }

        ret = shmring_pop(shm_ctx->ntsring_handle,
                          (char *)&node_idx, NODE_IDX_SIZE);
        retry_times++;
    }

    void *ptr = shm_offset_mem(shm_ctx->mp_handler, node_idx);
    *buf = (ntp_msg *)ptr;

    DEBUG("ntp_shm_recv success");
    return SUCCESS;
}

/**
 * bulk send: used by nt-monitor to send bulk message to libnts app
 */
int ntp_shm_send_bulk(
    ntp_shm_context_t shm_ctx, ntp_msg **bulk, size_t bulk_size)
{
    assert(shm_ctx);
    assert(bulk);
    assert(bulk_size > 0);

    shm_mempool_node *mp_node;
    for (size_t i = 0; i < bulk_size; i++)
    {
        mp_node = shm_mp_node_by_shmaddr(
            shm_ctx->mp_handler, (char *)bulk[i]);
        if (UNLIKELY(!mp_node))
        {
            ERR("cannot find corresponding shm_mempool_node in shm mempool");
            return FAILED;
        }

        shm_ctx->node_idxs_cache[i] = (char *)&mp_node->node_idx;
    }

    int rc;
    rc = shmring_push_bulk(shm_ctx->ntsring_handle,
                           shm_ctx->node_idxs_cache, shm_ctx->max_lens, bulk_size);
    while (rc != SUCCESS)
    {
        INFO("shmring_push_bulk failed, retry...");
        rc = shmring_push_bulk(shm_ctx->ntsring_handle,
                               shm_ctx->node_idxs_cache, shm_ctx->max_lens, bulk_size);
    }

    DEBUG("ntp_shm_send_bulk success");
    return rc;
}

/**
 * bulk recv: used by libnts app to receive message from nt-monitor
 */
size_t ntp_shm_recv_bulk(
    ntp_shm_context_t shm_ctx, ntp_msg **bulk, size_t bulk_size)
{
    assert(shm_ctx);
    assert(bulk);
    assert(bulk_size > 0);
    DEBUG("ntp_shm_recv_bulk enter...");

    size_t recv_cnt;
    recv_cnt = shmring_pop_bulk(shm_ctx->ntsring_handle,
                                shm_ctx->node_idxs_cache, shm_ctx->max_lens, bulk_size);
    DEBUG("shmring_pop_bulk with recv_cnt=%d", (int)recv_cnt);

    if (LIKELY(recv_cnt != FAILED))
    {
        for (size_t i = 0; i < recv_cnt; i++)
        {
            int node_idx;
            node_idx = *(int *)shm_ctx->node_idxs_cache[i];
            bulk[i] = (ntp_msg *)
                shm_offset_mem(shm_ctx->mp_handler, node_idx);
            if (UNLIKELY(!bulk[i]))
            {
                ERR("ntp_shm_recv_bulk failed");
                return FAILED;
            }
        }
    }

    DEBUG("ntp_shm_recv_bulk success");
    return recv_cnt;
}

int ntp_shm_send_bulk_idx(
    ntp_shm_context_t shm_ctx, char **node_idxs,
    size_t *idx_lens, size_t bulk_size)
{
    assert(shm_ctx);
    assert(node_idxs);
    assert(idx_lens);
    assert(bulk_size > 0);

    int rc;
    rc = shmring_push_bulk(shm_ctx->ntsring_handle,
                           node_idxs, idx_lens, bulk_size);
    while (rc != SUCCESS)
    {
        INFO("ntp_shm_send_bulk_idx failed, retry...");
        rc = shmring_push_bulk(shm_ctx->ntsring_handle,
                               node_idxs, idx_lens, bulk_size);
    }

    DEBUG("ntp_shm_send_bulk_idx success");
    return rc;
}

size_t ntp_shm_recv_bulk_idx(
    ntp_shm_context_t shm_ctx, char **node_idxs,
    size_t *max_lens, size_t bulk_size)
{
    assert(shm_ctx);
    assert(node_idxs);
    assert(max_lens);
    assert(bulk_size > 0);

    size_t recv_cnt;
    recv_cnt = shmring_pop_bulk(
        shm_ctx->ntsring_handle, node_idxs, max_lens, bulk_size);

    DEBUG("ntp_shm_recv_bulk_idx success");
    return recv_cnt;
}

int ntp_shm_front(
    ntp_shm_context_t shm_ctx, ntp_msg **buf)
{
    assert(shm_ctx);

    int ret;
    int node_idx = -1;
    ret = shmring_front(shm_ctx->ntsring_handle,
                        (char *)(int *)&node_idx, NODE_IDX_SIZE);

    if (ret == SUCCESS)
    {
        void *ptr = shm_offset_mem(shm_ctx->mp_handler, node_idx);
        assert(ptr);

        *buf = (ntp_msg *)ptr;

        DEBUG("ntp_shm_front success");
        return SUCCESS;
    }

    ERR("ntp_shm_front failed");
    return FAILED;
}

int ntp_shm_pop(ntp_shm_context_t shm_ctx) {
    assert(shm_ctx);

    return shmring_plain_pop(shm_ctx->ntsring_handle);
}

//创建者销毁
int ntp_shm_close(ntp_shm_context_t shm_ctx)
{
    assert(shm_ctx);

    if (shm_ctx->ntsring_handle)
    {
        shmring_free(shm_ctx->ntsring_handle, true);
        shm_ctx->ntsring_handle = NULL;
    }

    if (shm_ctx->mp_handler)
    {
        shm_mp_destroy(shm_ctx->mp_handler, 1);
        shm_ctx->mp_handler = NULL;
    }

    shm_ctx->shm_stat = NTP_SHM_UNLINK;

    DEBUG("ntp_shm_close success");
    return 0;
}

//使用者销毁
int ntp_shm_nts_close(ntp_shm_context_t shm_ctx)
{
    assert(shm_ctx);

    if (shm_ctx->ntsring_handle)
    {
        shmring_free(shm_ctx->ntsring_handle, false);
        shm_ctx->ntsring_handle = NULL;
    }

    if (shm_ctx->mp_handler)
    {
        shm_mp_destroy(shm_ctx->mp_handler, 0);
        shm_ctx->mp_handler = NULL;
    }

    shm_ctx->shm_stat = NTP_SHM_CLOSE;

    DEBUG("ntp_shm_ntm_close success");
    return 0;
}

void ntp_shm_destroy(ntp_shm_context_t shm_ctx)
{
    assert(shm_ctx);

    if (shm_ctx->node_idxs_cache)
    {
        free(shm_ctx->node_idxs_cache);
        shm_ctx->node_idxs_cache = NULL;
    }

    if (shm_ctx->node_idx_value_cache)
    {
        free(shm_ctx->node_idx_value_cache);
        shm_ctx->node_idx_value_cache = NULL;
    }

    if (shm_ctx->max_lens)
    {
        free(shm_ctx->max_lens);
        shm_ctx->max_lens = NULL;
    }

    free(shm_ctx);
    shm_ctx = NULL;
    DEBUG("ntp_shm_destroy success");
}

int ntp_shm_ntpacket_alloc(
    ntp_shm_context_t shm_ctx, ntp_msg **buf, size_t mtu_size)
{
    assert(shm_ctx);
    assert(buf);
    assert(mtu_size > 0);

    shm_mempool_node *mp_node;
    mp_node = shm_mp_malloc(shm_ctx->mp_handler, mtu_size);
    assert(mp_node);


    void *packet_ptr = shm_offset_mem(shm_ctx->mp_handler, mp_node->node_idx);
    assert(packet_ptr);
    
    *buf = (ntp_msg *)packet_ptr;

    DEBUG("ntp_shm_ntpacket_alloc success");
    return SUCCESS;
}

void ntp_shm_ntpacket_free(
    ntp_shm_context_t shm_ctx, ntp_msg **buf)
{
    assert(shm_ctx);
    assert(*buf);

    shm_mempool_node *tmp_node =
        shm_mp_node_by_shmaddr(shm_ctx->mp_handler, (char *)*buf);
    assert(tmp_node);

    shm_mp_free(shm_ctx->mp_handler, tmp_node);
}
