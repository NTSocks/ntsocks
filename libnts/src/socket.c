/*
 * <p>Title: socket.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */

#include "ntm_shm.h"
#include "socket.h"
#include "debug.h"

#define MSG_FROM_NTS "Hello, Nt-Monitor! I am libnts app."

int print_socket(){
	printf("Hello libnts app!\n");

	debug_print();

	printf("Bye, libnts app.\n");

	return 0;
}

int test_ntm_shm() {

	ntm_shm_context_t ntm_shm_ctx;
	char *ntm_shm_name = "/ntm-shm-ring";

	ntm_shm_ctx = ntm_shm();
	ntm_shm_connect(ntm_shm_ctx, ntm_shm_name, sizeof(ntm_shm_name));

	getchar();
	printf("Start to send message to Nt-Monitor. \n");

	int i;
	for (i = 0; i < 10; i++) {
		ntm_shm_send(ntm_shm_ctx, MSG_FROM_NTS, sizeof(MSG_FROM_NTS));
	}

	getchar();
	ntm_shm_nts_close(ntm_shm_ctx);
	ntm_shm_destroy(ntm_shm_ctx);

	return 0;
}
