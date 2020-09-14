/*
 * <p>Title: socket.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */
  
#include <string.h>

#include "ntm_shm.h"
#include "socket.h"
#include "nts_shm.h"


int print_socket(){
	printf("Hello libnts app!\n");

	printf("Bye, libnts app.\n");

	return 0;
}



int test_ntm_shm() {
	char *shm_name = "huangyibo";
	ntm_msg msg;
	msg.msg_id = 1357;
	msg.msg_type = NTM_MSG_NEW_SOCK;
	msg.sockid = 22;
	msg.domain = 1;
	msg.protocol = 2;
	msg.sock_type = 3;
	msg.nts_shm_addrlen = strlen(shm_name) + 1;
	memcpy(msg.nts_shm_name, shm_name, msg.nts_shm_addrlen);


	ntm_shm_context_t ntm_shm_ctx;
	char *ntm_shm_name = "/ntm-shm-ring";

	ntm_shm_ctx = ntm_shm();
	ntm_shm_connect(ntm_shm_ctx, ntm_shm_name, sizeof(ntm_shm_name));

	// getchar();
	printf("Start to send message to Nt-Monitor. \n");

	int i;
	for (i = 0; i < 10; i++) {
		msg.msg_id += i;
		ntm_shm_send(ntm_shm_ctx, &msg);
		printf("pop an element: msg_id-%d, msg_type=%d, sockid=%d, domain=%d, protocol=%d, sock_type=%d shmaddr=%s, nts_shm_addrlen=%d \n",
			   msg.msg_id, msg.msg_type,
			   msg.sockid, msg.domain,
			   msg.protocol, msg.sock_type,
			   msg.nts_shm_name, msg.nts_shm_addrlen);
	}

	getchar();
	ntm_shm_nts_close(ntm_shm_ctx);
	ntm_shm_destroy(ntm_shm_ctx);

	return 0;
}

void test_nts_shm() {	
	nts_msg msg = {
		.msg_id = 2,
		.msg_type = 3,
		.sockid = 4,
		.retval = 0
	};

	char *nts_shm_name = "nts-test";

	nts_shmring_handle_t handle;
	handle = nts_get_shmring(nts_shm_name, strlen(nts_shm_name));
	int i;
	bool ret;
	for ( i = 0; i < 10; i++)
	{
		msg.msg_id += i;
		ret = nts_shmring_push(handle, &msg);
		while (!ret)
		{
			// sched_yield();
			ret = nts_shmring_push(handle, &msg);
		}
		printf("msg_id=%d, msg_type=%d, sockid=%d, retval=%d\n", 
					msg.msg_id, msg.msg_type, msg.sockid, msg.retval);
		
	}
	
	nts_shmring_free(handle, 0);

}