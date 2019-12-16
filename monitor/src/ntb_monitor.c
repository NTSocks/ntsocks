/*
 * <p>Title: ntb-monitor.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 15, 2019 
 * @version 1.0
 */

#include <stdio.h>
#include <assert.h>

#include "ntb_monitor.h"
#include "config.h"
#include "ntm_shmring.h"
#include "ntm_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define MSG "Hello NT-Monitor! I am libnts app. Nice to meet you!"

static void * nts_recv_thread(void *arg);

static void * nts_send_thread(void *arg);

static void * ntp_recv_thread(void *arg);

static void * ntp_send_thread(void *arg);


static inline void test_ntm_ring() {
	ntm_shmring_handle_t ns_handle;

	ns_handle = ntm_shmring_init();
	ntm_shmring_push(ns_handle, MSG, sizeof(MSG));

	char data[256];
	size_t len = 256;

	ntm_shmring_free(ns_handle, 0);
	if (!ns_handle) {
		printf("free ntm shmring pass \n\n");
	}

	ns_handle = ntm_get_shmring();
	if (ns_handle) {
		printf("ntm get shmring pass \n");
	}

	int pop_msg_len;
	pop_msg_len = ntm_shmring_pop(ns_handle, data, len);

	printf("pop an element: data-%s, len-%d \n", data, pop_msg_len);

	ntm_shmring_free(ns_handle, 1);
}

static inline void test_ntm_shm() {
	ntm_shm_context_t ns_ctx;
	char *ntm_name = "/ntm-shm-ring";

	ns_ctx = ntm_shm();
	ntm_shm_accept(ns_ctx, ntm_name, sizeof(ntm_name));

	ntm_shm_send(ns_ctx, MSG, sizeof(MSG));
	char data[50];
	size_t len = 50;
	len = ntm_shm_recv(ns_ctx, data, len);
	printf("recv data: %s \n", data);

	ntm_shm_close(ns_ctx);
	ntm_shm_destroy(ns_ctx);
}

static inline void test_nts_shm() {
	ntm_shm_context_t ns_ctx;
	char *ntm_name = "/ntm-shm-ring";

	ns_ctx = ntm_shm();
	ntm_shm_accept(ns_ctx, ntm_name, sizeof(ntm_name));

	getchar();
	printf("Start to receive messages from libnts app...\n");

	char data[50];
	size_t len = 50;
	int recv_msg_len;
	for (int i = 0; i < 10; i++) {
		recv_msg_len = ntm_shm_recv(ns_ctx, data, len);
		printf("recv msg %d with length %d : %s \n", i+1, recv_msg_len, data);
	}

	getchar();
	ntm_shm_close(ns_ctx);
	ntm_shm_destroy(ns_ctx);

}


void * nts_recv_thread(void *arg) {
	assert(ntm_mgr->nts_ctx->shm_recv_ctx);

	DEBUG("nts_recv_thread ready...");

	char data[50];
	size_t len = 50;
	int recv_msg_len;
	for (int i = 0; i < 10; i++) {
		recv_msg_len = ntm_shm_recv(ntm_mgr->nts_ctx->shm_recv_ctx, data, len);
		printf("recv msg %d with length %d : %s \n", i+1, recv_msg_len, data);
	}

	DEBUG("nts_recv_thread end!");

	return 0;
}

void * nts_send_thread(void *arg) {

	return 0;
}

void * ntp_recv_thread(void *arg) {

	return 0;
}

void * ntp_send_thread(void *arg) {

	return 0;
}



ntm_manager_t get_ntm_manager() {

	return NULL;
}


int ntm_init(const char *config_file) {

	ntm_nts_context_t nts_ctx;
	ntm_ntp_context_t ntp_ctx;
	int ret;


	ntm_mgr = (ntm_manager_t) calloc(1, sizeof(struct ntm_manager));
	if(!ntm_mgr) {
		perror("malloc");
		ERR("Failed to allocate ntm_manager.");
		return -1;
	}
	ntm_mgr->nts_ctx = (ntm_nts_context_t) calloc(1, sizeof(struct ntm_nts_context));
	if (!ntm_mgr->nts_ctx) {
		perror("malloc");
		ERR("Failed to allocate ntm_nts_context.");
		return -1;
	}
	nts_ctx = ntm_mgr->nts_ctx;

	ntm_mgr->ntp_ctx = (ntm_ntp_context_t) calloc(1, sizeof(struct ntm_ntp_context));
	if(!ntm_mgr->ntp_ctx) {
		perror("malloc");
		ERR("Failed to allocate ntm_ntp_context.");
		return -1;
	}
	ntp_ctx = ntm_mgr->ntp_ctx;

	/**
	 * read conf file and init ntm params
	 */



	/**
	 *	init socket resources
	 */




	/**
	 * init the ntm shm ringbuffer to receive the messages from libnts apps
	 */
	ntm_mgr->nts_ctx->shm_recv_ctx = ntm_shm();
	if(!ntm_mgr->nts_ctx->shm_recv_ctx) {
		ERR("Failed to allocate ntm_shm_context.");
		return -1;
	}

	ret = ntm_shm_accept(ntm_mgr->nts_ctx->shm_recv_ctx,
			NTM_SHMRING_NAME, sizeof(NTM_SHMRING_NAME));
	if(ret) {
		ERR("ntm_shm_accept failed.");
		return -1;
	}

	pthread_create(&nts_ctx->shm_recv_thr, NULL, nts_recv_thread, NULL);





	/**
	 * init the listen socket for the connection requests from remote nt-monitor
	 */



	/**
	 * init the connection status hashmap store for remote nt-monitor
	 */


	return 0;
}

void ntm_destroy() {
	assert(ntm_mgr);

	/**
	 * Destroy the ntm shmring resources
	 */
	ntm_shm_context_t shm_recv_ctx;
	shm_recv_ctx = ntm_mgr->nts_ctx->shm_recv_ctx;
	if(shm_recv_ctx && shm_recv_ctx->shm_stat == NTM_SHM_READY) {
		ntm_shm_close(shm_recv_ctx);
		ntm_shm_destroy(shm_recv_ctx);
	}

	nts_shm_context_t shm_send_ctx;
	shm_send_ctx = ntm_mgr->nts_ctx->shm_send_ctx;
	if (shm_send_ctx && shm_send_ctx->shm_stat == NTS_SHM_READY) {
		nts_shm_ntm_close(shm_send_ctx);
		nts_shm_destroy(shm_send_ctx);
	}

	free(ntm_mgr->ntp_ctx);
	free(ntm_mgr->nts_ctx);

	free(ntm_mgr);

}

int ntm_getconf(struct ntm_config *conf) {

	return 0;
}

int ntm_setconf(const struct ntm_config *conf) {

	return 0;
}

ntm_manager_t init_ntm_manager() {

	return NULL;
}


int print_monitor() {
	printf("Hello Ntb Monitor.\n");

//	test_ntm_ring();
//	test_ntm_shm();
	test_nts_shm();
	printf("Bye, Ntb Monitor.\n");

	return 0;
}
