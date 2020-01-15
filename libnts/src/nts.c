/*
 * <p>Title: nts.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "nts.h"
#include "config.h"
#include "nts_config.h"
#include "nt_log.h"
#include "nt_errno.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);


nts_context_t get_nts_context() {

    return NULL;
}


int nts_context_init(const char *config_file) {
    DEBUG("init nts_context");

    /**
     * read nts-related config file and init libnts params
     */
    print_conf();
    DEBUG("load the libnts config file");
    load_conf(config_file);
    DEBUG("the libnts config file loaded");
    print_conf();

    int ret;
    nts_ntm_context_t ntm_ctx;
    nts_ntp_context_t ntp_ctx;

    nts_ctx = (nts_context_t) calloc(1, sizeof(struct nts_context));
    const char *err_msg = "malloc";
    if(!nts_ctx) {
        perror(err_msg);
        ERR("Failed to allocate nts_context.");
        return -1;
    }

    // init hash map for the mapping between nt_socket_id and nt_sock_context_t
    nts_ctx->nt_sock_map = createHashMap(NULL, NULL);

    nts_ctx->ntm_ctx = (nts_ntm_context_t) calloc(1, sizeof(struct nts_ntm_context));
    if(!nts_ctx->ntm_ctx) {
        perror(err_msg);
        ERR("Failed to allocate nts_ntm_context.");
		return -1;
    }
    ntm_ctx = nts_ctx->ntm_ctx;

    nts_ctx->ntp_ctx = (nts_ntp_context_t)calloc(1, sizeof(struct nts_ntp_context));
    if(!nts_ctx->ntp_ctx) {
        perror(err_msg);
        ERR("Failed to allocate nts_ntp_context.");
		return -1;
    }
    ntp_ctx = nts_ctx->ntp_ctx;


    /**
     * init/get the ntm shm ring queue to send the control messages 
     * from libnts to ntb monitor
     */
    ntm_ctx->shm_send_ctx = ntm_shm();
    if(!ntm_ctx->shm_send_ctx) {
        ERR("Failed to init/get ntm_shm_context.");
		return -1;
    }

    ret = ntm_shm_connect(ntm_ctx->shm_send_ctx, 
                        NTM_SHMRING_NAME, sizeof(NTM_SHMRING_NAME));
    if(ret) {
        ERR("ntm_shm_connect failed.");
		return -1;
    }


    DEBUG("nts_context_init pass");

    return 0;
}


void nts_context_destroy() {
    assert(nts_ctx);

    DEBUG("destroy nts_context ready...");

    /**
     * destroy the ntm shm ring queue between libnts and ntb monitor
     */
    if (nts_ctx->ntm_ctx->shm_send_ctx && 
            nts_ctx->ntm_ctx->shm_send_ctx->shm_stat == NTM_SHM_READY) {
        ntm_shm_nts_close(nts_ctx->ntm_ctx->shm_send_ctx);
        ntm_shm_destroy(nts_ctx->ntm_ctx->shm_send_ctx);
    }
    DEBUG("destroy the ntm_shm_context_t shm_send_ctx resource pass");

    /**
     * destroy the ntp context resources, including:
     */
    DEBUG("destroy the ntp context resources");


    /**
     * destroy the nt_socket-level nt_sock_context_t for each established nt_socket
     */
    HashMapIterator iter;
    iter = createHashMapIterator(nts_ctx->nt_sock_map);
    nt_sock_context_t nt_sock_ctx;
    while (hasNextHashMapIterator(iter))
    {
        DEBUG("iter next");
        iter = nextHashMapIterator(iter);
        nt_sock_ctx = iter->entry->value;
        
        /**
         * destroy one nt_sock_context_t(equal to the workflow of `close()`): 
         * TODO:0. update local socket state, notify the ntm to destroy nt_socket resources
         * TODO:1. close/destroy the send/recv ntp shm ring queue
         * 2. close/destroy the nts shm ring queue between nt_socket and ntm
         */
        DEBUG("0. update local socket state, notify the ntm to destroy nt_socket resources");

        DEBUG("1. close/destroy the send/recv ntp shm ring queue");

        DEBUG("2. close/destroy the nts shm ring queue between nt_socket and ntm");
        if(nt_sock_ctx->nts_shm_ctx && 
                nt_sock_ctx->nts_shm_ctx->shm_stat == NTS_SHM_READY) {
            nts_shm_close(nt_sock_ctx->nts_shm_ctx);
            nts_shm_destroy(nt_sock_ctx->nts_shm_ctx);
        }

        if (nt_sock_ctx->socket) {
            free(nt_sock_ctx->socket);
        }

        Remove(nts_ctx->nt_sock_map, &nt_sock_ctx->socket->sockid);

        DEBUG("free the nt_socket and nts_ntm_context");
        // free(nt_sock_ctx->socket);
        free(nt_sock_ctx);
    }
    freeHashMapIterator(&iter);
    Clear(nts_ctx->nt_sock_map);
    DEBUG("destroy nt_sock_map pass");
    
    if(nts_ctx->ntp_ctx) {
         free(nts_ctx->ntp_ctx);
         nts_ctx->ntp_ctx = NULL;
         DEBUG("free nts_ctx->ntp_ctx pass");
    }
    if (nts_ctx->ntm_ctx) {
        free(nts_ctx->ntm_ctx);
        nts_ctx->ntm_ctx = NULL;
        DEBUG("free nts_ctx->ntm_ctx pass");
    }
    DEBUG("free(nts_ctx);");
    free(nts_ctx);

    /**
     * destroy the memory which is allocated to NTS_CONFIG
     */
    free_conf();

    DEBUG("destroy nts_context pass");
}

int generate_nts_shmname(char * nts_shmaddr) {
    assert(nts_shmaddr);

    char *shm_uuid = generate_uuid();
	sprintf(nts_shmaddr, "nts_shm-%s", shm_uuid);
	free(shm_uuid);

    return 0;
}

int nt_sock_errno(int sockid){
    assert(sockid > 0);

    nt_sock_context_t nt_sock_ctx;
	nt_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx){
        ERR("Non-existing sockid or invaild sockid");
        return -1;
    }

    return nt_sock_ctx->err_no;
}
