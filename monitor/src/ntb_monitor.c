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

#include "ntb_monitor.h"
#include "ntm_shmring.h"
#include "ntm_shm.h"

#define MSG "Hello NT-Monitor! I am libnts app. Nice to meet you!"

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

int print_monitor() {
	printf("Hello Ntb Monitor.\n");

//	test_ntm_ring();
//	test_ntm_shm();
	test_nts_shm();
	printf("Bye, Ntb Monitor.\n");

	return 0;
}
