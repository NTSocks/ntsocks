/*
 * <p>Title: ntp2nts_shm.c</p>
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

#include "ntp2nts_shm.h"
#include "utils.h"
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
	shm_ctx->ntsring_handle = shmring_init(shm_ctx->shm_addr, shm_ctx->addrlen);
    if(!shm_ctx->ntsring_handle) {
        ERR("init shmring failed.");
        goto FAIL;
    }

    // for bulk operations on shmring
    shm_ctx->node_idxs_cache = (char **) calloc(MAX_BUFS, sizeof(char *));
    if (!shm_ctx->node_idxs_cache) {
        ERR("malloc shm_ctx->node_idxs_cache failed.");
        goto FAIL;
    }
    shm_ctx->node_idx_value_cache = (int *)calloc(MAX_BUFS, sizeof(int));
    if (!shm_ctx->node_idx_value_cache) {
        ERR("malloc shm_ctx->node_idx_value_cache failed.");
        goto FAIL;
    }
    shm_ctx->max_lens = (size_t *) calloc(MAX_BUFS, sizeof(size_t));
    if (!shm_ctx->max_lens) {
        ERR("malloc shm_ctx->max_lens failed.");
        goto FAIL;
    }

    for (int i = 0; i < MAX_BUFS; i++)
    {
        shm_ctx->max_lens[i] = sizeof(int);
        shm_ctx->node_idxs_cache[i] = (char *) &(shm_ctx->node_idx_value_cache[i]);
    }
    

	
    /* init shm mempool */
    char mp_name[30] = {0};
    sprintf(mp_name, "%s-mp", shm_addr);
    shm_ctx->mp_handler = shm_mp_init(sizeof(ntp_msg), MAX_BUFS + 8, mp_name, strlen(mp_name));
    if(!shm_ctx->mp_handler) {
        ERR("init shm mempool failed. ");
        goto FAIL;
    }
    
    shm_ctx->shm_stat = NTP_SHM_READY;

	DEBUG("ntp_shm_accept pass");
	return 0;

    FAIL: 
    if(shm_ctx->mp_handler) {
        shm_mp_destroy(shm_ctx->mp_handler, 1);
        shm_ctx->mp_handler = NULL;
    }

    if(shm_ctx->node_idxs_cache) {
        free(shm_ctx->node_idxs_cache);
        shm_ctx->node_idxs_cache = NULL;
    }

    if (shm_ctx->node_idx_value_cache) {
        free(shm_ctx->node_idx_value_cache);
        shm_ctx->node_idx_value_cache = NULL;
    }

    if (shm_ctx->max_lens) {
        free(shm_ctx->max_lens);
        shm_ctx->max_lens = NULL;
    }

    if(shm_ctx->ntsring_handle) {
        shmring_free(shm_ctx->ntsring_handle, 1);
        shm_ctx->ntsring_handle = NULL;
    }

    if(shm_ctx->shm_addr) {
        free(shm_ctx->shm_addr);
	    shm_ctx->shm_addr = NULL;
    }

    return -1;
}


int ntp_shm_connect(ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen) {
	assert(shm_ctx);
	assert(addrlen > 0);

	shm_ctx->shm_addr = (char *)malloc(addrlen);
    if(!shm_ctx->shm_addr){
        perror("malloc");
        return -1;
    }
	memset(shm_ctx->shm_addr, 0, addrlen);
	shm_ctx->addrlen = addrlen;
	memcpy(shm_ctx->shm_addr, shm_addr, addrlen);
	shm_ctx->ntsring_handle = get_shmring(shm_ctx->shm_addr, shm_ctx->addrlen); /// note: need to improve
	if(!shm_ctx->ntsring_handle) {
        ERR("get shmring failed.");
        goto FAIL;
    }

    // for bulk operations on shmring
    shm_ctx->node_idxs_cache = (char **) calloc(MAX_BUFS, sizeof(char *));
    if (!shm_ctx->node_idxs_cache) {
        ERR("malloc shm_ctx->node_idxs_cache failed.");
        goto FAIL;
    }
    shm_ctx->node_idx_value_cache = (int *)calloc(MAX_BUFS, sizeof(int));
    if (!shm_ctx->node_idx_value_cache) {
        ERR("malloc shm_ctx->node_idx_value_cache failed.");
        goto FAIL;
    }
    shm_ctx->max_lens = (size_t *) calloc(MAX_BUFS, sizeof(size_t));
    if (!shm_ctx->max_lens) {
        ERR("malloc shm_ctx->max_lens failed.");
        goto FAIL;
    }

    for (int i = 0; i < MAX_BUFS; i++)
    {
        shm_ctx->max_lens[i] = sizeof(int);
        shm_ctx->node_idxs_cache[i] = (char *) &(shm_ctx->node_idx_value_cache[i]);
    }


    /* init shm mempool */
    char mp_name[30] = {0};
    sprintf(mp_name, "%s-mp", shm_addr);
    shm_ctx->mp_handler = shm_mp_init(sizeof(ntp_msg), MAX_BUFS + 8, mp_name, strlen(mp_name));
    if(!shm_ctx->mp_handler) {
        ERR("init shm mempool failed. ");
        goto FAIL;
    }
    
    
    shm_ctx->shm_stat = NTP_SHM_READY;

	DEBUG("ntp_shm_connect pass");
	return 0;

    FAIL: 

    if(shm_ctx->mp_handler) {
        shm_mp_destroy(shm_ctx->mp_handler, 0);
        shm_ctx->mp_handler = NULL;
    }

    if(shm_ctx->node_idxs_cache) {
        free(shm_ctx->node_idxs_cache);
        shm_ctx->node_idxs_cache = NULL;
    }

    if (shm_ctx->node_idx_value_cache) {
        free(shm_ctx->node_idx_value_cache);
        shm_ctx->node_idx_value_cache = NULL;
    }

    if (shm_ctx->max_lens) {
        free(shm_ctx->max_lens);
        shm_ctx->max_lens = NULL;
    }

    if(shm_ctx->ntsring_handle) {
        shmring_free(shm_ctx->ntsring_handle, 0);
        shm_ctx->ntsring_handle = NULL;
    }

    if(shm_ctx->shm_addr) {
        free(shm_ctx->shm_addr);
	    shm_ctx->shm_addr = NULL;
    }

    return -1;

}


int ntp_shm_send(ntp_shm_context_t shm_ctx, ntp_msg *buf) {
	assert(shm_ctx);
    assert(buf);

	bool ret;
	DEBUG("shmring_push invoked");

    shm_mempool_node * mp_node;
    mp_node = shm_mp_node_by_shmaddr(shm_ctx->mp_handler, (char *)buf);
    if(!mp_node) {
        ERR("cannot find corresponding shm_mempool_node in shm mempool with ntp_msg shm addr=%p", buf);
        return -1;
    }

    DEBUG("shmring push node_idx=%d", mp_node->node_idx);
	ret = shmring_push(shm_ctx->ntsring_handle, (char *) &mp_node->node_idx, NODE_IDX_SIZE);
	//TODO: add the timeout limit for retry push times.
    // int retry_times = 1;
	while(!ret) {
        // once the retry_times is greater than pre-define 'RETRY_TIMES' (default 5), 
        //  indicate current ntp_shm ring is empty and immediately return NULL
        // if(retry_times > RETRY_TIMES) 
        //     break;

		sched_yield();
		INFO("shmring_push failed and maybe shmring is full.");
		ret = shmring_push(shm_ctx->ntsring_handle, (char *) &mp_node->node_idx, NODE_IDX_SIZE);
        // retry_times ++;
	}

	DEBUG("ntp_shm_send pass with ret=%d", ret);
	return ret ? SUCCESS : FAILED;
}


ntp_msg * ntp_shm_recv(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

	bool ret;
    int node_idx = -1;
	ret = shmring_pop(shm_ctx->ntsring_handle, (char*)&node_idx, NODE_IDX_SIZE);
	//TODO: add the timeout limit for retry pop times.
    int retry_times = 1;
	while(!ret) {
        // once the retry_times is greater than pre-define 'RETRY_TIMES' (default 5), 
        //  indicate current ntp_shm ring is empty and immediately return NULL
        if(retry_times > RETRY_TIMES){
            INFO("ntp_shm_recv failed: the ntp_shm_recv retry times > RETRY_TIME(%d)", RETRY_TIMES);
            return NULL;
        }

		sched_yield();
		ret = shmring_pop(shm_ctx->ntsring_handle, (char*)&node_idx, NODE_IDX_SIZE);
        retry_times ++;
	}

    DEBUG("node_idx=%d", node_idx);

    ntp_msg *buf;
    if(ret) {
        buf = (ntp_msg *) shm_offset_mem(shm_ctx->mp_handler, node_idx);
        if (UNLIKELY(!buf)) {
            ERR("ntp_shm_recv failed");
            return NULL;
        }
        DEBUG("ntp_shm_recv success");
        return buf;
    }

	ERR("ntp_shm_recv failed");
	return NULL;
}



/**
 * bulk send: used by nt-monitor to send bulk message to libnts app
 */
int ntp_shm_send_bulk(ntp_shm_context_t shm_ctx, ntp_msg **bulk, size_t bulk_size) {
    assert(shm_ctx);
    assert(bulk);
    assert(bulk_size > 0);

    shm_mempool_node * mp_node;
    for (size_t i = 0; i < bulk_size; i++)
    {
        mp_node = shm_mp_node_by_shmaddr(shm_ctx->mp_handler, (char *) bulk[i]);
        if(UNLIKELY(!mp_node)) {
            ERR("cannot find corresponding shm_mempool_node in shm mempool");
            return FAILED;
        }

        shm_ctx->node_idxs_cache[i] = (char *) &mp_node->node_idx;
    }
    
    bool rc;
    rc = shmring_push_bulk(shm_ctx->ntsring_handle, shm_ctx->node_idxs_cache, shm_ctx->max_lens, bulk_size);
    while (!rc)
    {
        INFO("shmring_push_bulk failed, retry...");
        sched_yield();
        rc = shmring_push_bulk(shm_ctx->ntsring_handle, shm_ctx->node_idxs_cache, shm_ctx->max_lens, bulk_size);
    }
    
    DEBUG("ntp_shm_send_bulk success");
    return rc ? SUCCESS : FAILED;
}

/**
 * bulk recv: used by libnts app to receive message from nt-monitor
 */
size_t ntp_shm_recv_bulk(ntp_shm_context_t shm_ctx, ntp_msg **bulk, size_t bulk_size) {
    assert(shm_ctx);
    assert(bulk);
    assert(bulk_size > 0);

    size_t recv_cnt;
    recv_cnt = shmring_pop_bulk(shm_ctx->ntsring_handle, shm_ctx->node_idxs_cache, shm_ctx->max_lens, bulk_size);
    
    if (LIKELY(recv_cnt != FAILED)) {
        for (size_t i = 0; i < recv_cnt; i++)
        {
            int node_idx;
            node_idx = *(int *)shm_ctx->node_idxs_cache[i];
            bulk[i] = (ntp_msg *) shm_offset_mem(shm_ctx->mp_handler, node_idx);
            if (UNLIKELY(!bulk[i])) {
                ERR("ntp_shm_recv_bulk failed");
                return FAILED;
            }
        }
    }

    DEBUG("ntp_shm_recv_bulk success");
    return recv_cnt;
}


int ntp_shm_send_bulk_idx(ntp_shm_context_t shm_ctx, char **node_idxs, size_t *idx_lens, size_t bulk_size) {
    assert(shm_ctx);
    assert(node_idxs);
    assert(idx_lens);
    assert(bulk_size > 0);

    bool rc;
    rc = shmring_push_bulk(shm_ctx->ntsring_handle, node_idxs, idx_lens, bulk_size);
    while (!rc)
    {
        INFO("ntp_shm_send_bulk_idx failed, retry...");
        sched_yield();
        rc = shmring_push_bulk(shm_ctx->ntsring_handle, node_idxs, idx_lens, bulk_size);
    }

    DEBUG("ntp_shm_send_bulk_idx success");
    return rc ? SUCCESS : FAILED;
}


size_t ntp_shm_recv_bulk_idx(ntp_shm_context_t shm_ctx, char **node_idxs, size_t *max_lens, size_t bulk_size) {
    assert(shm_ctx);
    assert(node_idxs);
    assert(max_lens);
    assert(bulk_size > 0);

    size_t recv_cnt;
    recv_cnt = shmring_pop_bulk(shm_ctx->ntsring_handle, node_idxs, max_lens, bulk_size);
    
    DEBUG("ntp_shm_recv_bulk_idx success");
    return recv_cnt;
}



ntp_msg * ntp_shm_front(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

    ntp_msg *buf;

	bool ret;
    int node_idx = -1;
	ret = shmring_front(shm_ctx->ntsring_handle, (char*)(int*)&node_idx, NODE_IDX_SIZE);
    DEBUG("node_idx=%d", node_idx);

    if(ret) {
        buf = (ntp_msg *) shm_offset_mem(shm_ctx->mp_handler, node_idx);
        if (!buf) {
            ERR("[in]ntp_shm_front failed");
            return NULL;
        }
        DEBUG("ntp_shm_front success");
        return buf;
    }


	// DEBUG("[out]ntp_shm_front failed");
	return NULL;
}

//创建者销毁
int ntp_shm_close(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

    if(shm_ctx->ntsring_handle) {
        shmring_free(shm_ctx->ntsring_handle, 1);
        shm_ctx->ntsring_handle = NULL;
    }

    if(shm_ctx->mp_handler) {
        shm_mp_destroy(shm_ctx->mp_handler, 1);
        shm_ctx->mp_handler = NULL;
    }

    if(shm_ctx->shm_addr) {
        free(shm_ctx->shm_addr);
	    shm_ctx->shm_addr = NULL;
    }

	shm_ctx->shm_stat = NTP_SHM_UNLINK;

	DEBUG("ntp_shm_close pass");
	return 0;
}

//使用者销毁
int ntp_shm_nts_close(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

    if(shm_ctx->ntsring_handle) {
        shmring_free(shm_ctx->ntsring_handle, 0);
        shm_ctx->ntsring_handle = NULL;
    }

    if(shm_ctx->mp_handler) {
        shm_mp_destroy(shm_ctx->mp_handler, 0);
        shm_ctx->mp_handler = NULL;
    }

    if(shm_ctx->shm_addr) {
        free(shm_ctx->shm_addr);
	    shm_ctx->shm_addr = NULL;
    }

	shm_ctx->shm_stat = NTP_SHM_CLOSE;

	DEBUG("ntp_shm_ntm_close pass");
	return 0;
}


void ntp_shm_destroy(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

	free(shm_ctx);
	shm_ctx = NULL;
	DEBUG("ntp_shm_destroy pass");
}

