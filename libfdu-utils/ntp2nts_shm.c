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
#include "nt_log.h"


DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

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
	ret = shmring_push(shm_ctx->ntsring_handle, (char *)(int*) &mp_node->node_idx, NODE_IDX_SIZE);
	//TODO: add the timeout limit for retry push times.
	while(!ret) {
		sched_yield();
		INFO("shmring_push failed and maybe shmring is full.");
		ret = shmring_push(shm_ctx->ntsring_handle, (char *)(int*) &mp_node->node_idx, NODE_IDX_SIZE);
	}

	DEBUG("ntp_shm_send pass with ret=%d", ret);
	return ret ? 0 : -1;
}


ntp_msg * ntp_shm_recv(ntp_shm_context_t shm_ctx) {
	assert(shm_ctx);

	bool ret;
    int node_idx;
	ret = shmring_pop(shm_ctx->ntsring_handle, (char*)(int*)&node_idx, NODE_IDX_SIZE);
	//TODO: add the timeout limit for retry pop times.
	while(!ret) {
		sched_yield();
		ret = shmring_pop(shm_ctx->ntsring_handle, (char*)(int*)&node_idx, NODE_IDX_SIZE);
	}

    DEBUG("node_idx=%d", node_idx);

    ntp_msg *buf;
    if(ret) {
        buf = (ntp_msg *) shm_offset_mem(shm_ctx->mp_handler, node_idx);
        if (!buf) {
            ERR("ntp_shm_recv failed");
            return NULL;
        }
        DEBUG("ntp_shm_recv success");
        return buf;
    }

	ERR("ntp_shm_recv failed");
	return NULL;
}

int ntp_shm_front(ntp_shm_context_t shm_ctx, ntp_msg *buf) {
	assert(shm_ctx);

	bool ret;
    int node_idx;
	ret = shmring_front(shm_ctx->ntsring_handle, (char*)(int*)&node_idx, NODE_IDX_SIZE);
    if(ret) {
        buf = (ntp_msg *) shm_offset_mem(shm_ctx->mp_handler, node_idx);
        if (!buf) {
            ERR("ntp_shm_recv failed");
            return -1;
        }
        DEBUG("ntp_shm_recv success");
        return 0;
    }


	DEBUG("ntp_shm_front failed");
	return -1;
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

