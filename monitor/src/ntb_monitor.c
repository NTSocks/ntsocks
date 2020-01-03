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
#include <arpa/inet.h>

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
//	ntm_shmring_handle_t ns_handle;
//	char *ntm_name = "/ntm-shm-ring";
//
//	ns_handle = ntm_shmring_init(ntm_name, sizeof(ntm_name));
//	ntm_shmring_push(ns_handle, MSG, sizeof(MSG));
//
//	char data[256];
//	size_t len = 256;
//
//	ntm_shmring_free(ns_handle, 0);
//	if (!ns_handle) {
//		printf("free ntm shmring pass \n\n");
//	}
//
//	ns_handle = ntm_get_shmring(ntm_name, sizeof(ntm_name));
//	if (ns_handle) {
//		printf("ntm get shmring pass \n");
//	}
//
//	int pop_msg_len;
//	pop_msg_len = ntm_shmring_pop(ns_handle, data, len);
//
//	printf("pop an element: data-%s, len-%d \n", data, pop_msg_len);
//
//	ntm_shmring_free(ns_handle, 1);
}

static inline void test_ntm_shm() {
//	ntm_shm_context_t ns_ctx;
//	char *ntm_name = "/ntm-shm-ring";
//
//	ns_ctx = ntm_shm();
//	ntm_shm_accept(ns_ctx, ntm_name, sizeof(ntm_name));
//
//	ntm_shm_send(ns_ctx, MSG, sizeof(MSG));
//	char data[50];
//	size_t len = 50;
//	len = ntm_shm_recv(ns_ctx, data, len);
//	printf("recv data: %s \n", data);
//
//	ntm_shm_close(ns_ctx);
//	ntm_shm_destroy(ns_ctx);
}

static inline void test_nts_shm() {
//	ntm_shm_context_t ns_ctx;
//	char *ntm_name = "/ntm-shm-ring";
//
//	ns_ctx = ntm_shm();
//	ntm_shm_accept(ns_ctx, ntm_name, sizeof(ntm_name));
//
//	getchar();
//	printf("Start to receive messages from libnts app...\n");
//
//	char data[50];
//	size_t len = 50;
//	int recv_msg_len;
//	for (int i = 0; i < 10; i++) {
//		recv_msg_len = ntm_shm_recv(ns_ctx, data, len);
//		printf("recv msg %d with length %d : %s \n", i + 1, recv_msg_len, data);
//	}
//
//	getchar();
//	ntm_shm_close(ns_ctx);
//	ntm_shm_destroy(ns_ctx);

}

void * nts_recv_thread(void *arg) {
	assert(ntm_mgr->nts_ctx->shm_recv_ctx);

	DEBUG("nts_recv_thread ready...");

//	char data[50];
//	size_t len = 50;
//	int recv_msg_len;
//	for (int i = 0; i < 10; i++) {
//		recv_msg_len = ntm_shm_recv(ntm_mgr->nts_ctx->shm_recv_ctx, data, len);
//		printf("recv msg %d with length %d : %s \n", i+1, recv_msg_len, data);
//	}

	DEBUG("nts_recv_thread end!");

	return 0;
}

void * nts_send_thread(void *arg) {

	DEBUG("ntm_sock_recv_thread ready...");

	DEBUG("ntm_sock_recv_thread end!");

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

	/**
	 * read conf file and init ntm params
	 */
	DEBUG("load the ntm config file");


	ntm_nts_context_t nts_ctx;
	ntm_ntp_context_t ntp_ctx;
	ntm_conn_ctx_t ntm_conn_ctx;
	nt_listener_context_t nt_listener_ctx;
	int ret;

	ntm_mgr = (ntm_manager_t) calloc(1, sizeof(struct ntm_manager));
	const char *err_msg = "malloc";
	if (!ntm_mgr) {
		perror(err_msg);
		ERR("Failed to allocate ntm_manager.");
		return -1;
	}


	ntm_mgr->nts_ctx = (ntm_nts_context_t) calloc(1,
			sizeof(struct ntm_nts_context));
	if (!ntm_mgr->nts_ctx) {
		perror(err_msg);
		ERR("Failed to allocate ntm_nts_context.");
		return -1;
	}
	nts_ctx = ntm_mgr->nts_ctx;
	// hash map for cache the nts shm ringbuffer context created by nts
	nts_ctx->nts_shm_conn_map = createHashMap(NULL, NULL);


	ntm_mgr->ntp_ctx = (ntm_ntp_context_t) calloc(1,
			sizeof(struct ntm_ntp_context));
	if (!ntm_mgr->ntp_ctx) {
		perror(err_msg);
		ERR("Failed to allocate ntm_ntp_context.");
		return -1;
	}
	ntp_ctx = ntm_mgr->ntp_ctx;


	ntm_mgr->ntm_conn_ctx = (ntm_conn_ctx_t) calloc(1,
			sizeof(struct ntm_conn_context));
	if (!ntm_mgr->ntm_conn_ctx) {
		perror(err_msg);
		ERR("Failed to allocate ntm_ntp_context.");
		return -1;
	}
	ntm_conn_ctx = ntm_mgr->ntm_conn_ctx;
	// hash map for cache the remote ntm connection list
	ntm_conn_ctx->conn_map = createHashMap(NULL, NULL);


	ntm_mgr->nt_listener_ctx = (nt_listener_context_t) calloc (1,
			sizeof(struct nt_listener_context));
	if (!ntm_mgr->nt_listener_ctx) {
		perror(err_msg);
		ERR("Failed to allocate ntm_ntp_context.");
		return -1;
	}
	nt_listener_ctx = ntm_mgr->nt_listener_ctx;
	// hash map for holding all nt_listener indexed by port
	nt_listener_ctx->listener_map = createHashMap(NULL, NULL);


	/**
	 *	init global socket and port resources
	 */
	DEBUG("Init global socket and port resources");




	/**
	 * init the ntm shm ringbuffer to receive the messages from libnts apps
	 */
	ntm_mgr->nts_ctx->shm_recv_ctx = ntm_shm();
	if (!ntm_mgr->nts_ctx->shm_recv_ctx) {
		ERR("Failed to allocate ntm_shm_context.");
		return -1;
	}

	ret = ntm_shm_accept(ntm_mgr->nts_ctx->shm_recv_ctx,
	NTM_SHMRING_NAME, sizeof(NTM_SHMRING_NAME));
	if (ret) {
		ERR("ntm_shm_accept failed.");
		return -1;
	}

	pthread_create(&nts_ctx->shm_recv_thr, NULL, nts_recv_thread, NULL);

	/**
	 * init the listen socket for the connection requests from remote nt-monitor
	 */
	ntm_conn_ctx->listen_ip = NTM_CONFIG.listen_ip;
	ntm_conn_ctx->listen_port = NTM_CONFIG.listen_port;
	ntm_conn_ctx->addrlen = NTM_CONFIG.ipaddr_len;
	ntm_conn_ctx->stop_signal = 0;
	ntm_conn_ctx->server_sock = ntm_sock_create();
	pthread_create(&ntm_conn_ctx->ntm_sock_listen_thr, NULL,
			ntm_sock_listen_thread, ntm_conn_ctx);
//	pthread_create(&(ntm_conn_ctx->ntm_sock_listen_thr), NULL,
//			nts_send_thread, NULL);


	DEBUG("ntm_init pass");

	return 0;
}

void ntm_destroy() {
	assert(ntm_mgr);
	DEBUG("nt_destroy ready...");

	/**
	 * Destroy the ntp context resources
	 */
	DEBUG("Destroy ntp context resources");


	/**
	 * Destroy the ntm listen socket resources for the communication
	 * between local and remote ntb-monitor
	 */
	HashMapIterator iter;
	ntm_conn_t ntm_conn;
	iter = createHashMapIterator(ntm_mgr->ntm_conn_ctx->conn_map);
	while (hasNextHashMapIterator(iter)) {
		DEBUG("iter next");
		iter = nextHashMapIterator(iter);
		ntm_conn = iter->entry->value;
		ntm_close_socket(ntm_conn->client_sock);
		Remove(ntm_mgr->ntm_conn_ctx->conn_map, ntm_conn->ip);
	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->ntm_conn_ctx->conn_map);
	DEBUG("free hashmap for ntm_conn context pass");

	if(ntm_mgr->ntm_conn_ctx->server_sock) {
		DEBUG("close server_sock");
		ntm_close_socket(ntm_mgr->ntm_conn_ctx->server_sock);
	}
//	if(ntm_mgr->ntm_conn_ctx->listen_ip) {
//		DEBUG("free listen_ip");
//		free(ntm_mgr->ntm_conn_ctx->listen_ip);
//	}
	DEBUG("Destroy ntm listen socket resources pass");



	/**
	 * Destroy the nt_listener related resources for the ntb-based
	 * listen socket
	 */
	HashMapIterator nt_conn_iter;
	nt_listener_warpper_t nt_listener_warpper;
	nt_accepted_conn_t nt_accepted_conn;
	iter = createHashMapIterator(ntm_mgr->nt_listener_ctx->listener_map);
	while (hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		nt_listener_warpper = iter->entry->value;

		nt_conn_iter = createHashMapIterator(nt_listener_warpper->accepted_conn_map);

		while (hasNextHashMapIterator(nt_conn_iter)) {
			nt_conn_iter = nextHashMapIterator(iter);
			nt_accepted_conn = nt_conn_iter->entry->value;

			// free or unbound socket id
			free_socket(nt_accepted_conn->sockid, 1);
			Remove(nt_listener_warpper->accepted_conn_map, nt_accepted_conn->sockid);

		}
		freeHashMapIterator(&nt_conn_iter);
		Clear(nt_listener_warpper->accepted_conn_map);
		DEBUG("free hash map for client socket connection accepted by the specified nt_listener socket pass");

		// free or unbound nt_listener socket id
		free_socket(nt_listener_warpper->listener->sockid, 1);
		Remove(ntm_mgr->nt_listener_ctx->listener_map, nt_listener_warpper->port);

	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->nt_listener_ctx->listener_map);
	DEBUG("Destroy the nt_listener related resources pass");


	/**
	 * Destroy the ntm-nts context resources
	 */
	nts_shm_conn_t nts_shm_conn;
	iter = createHashMapIterator(ntm_mgr->nts_ctx->nts_shm_conn_map);
	while (hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		nts_shm_conn = iter->entry->value;

		// free or unbound socket id
		free_socket(nts_shm_conn->sockid, 1);

		// free nts_shm_context
		if(nts_shm_conn->nts_shm_ctx
				&& nts_shm_conn->nts_shm_ctx->shm_stat == NTS_SHM_READY) {
			nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
			nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
		}

		Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, nts_shm_conn->sockid);

	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->nts_ctx->nts_shm_conn_map);
	DEBUG("free hash map for nts_shm_conn pass");

	ntm_shm_context_t shm_recv_ctx;
	shm_recv_ctx = ntm_mgr->nts_ctx->shm_recv_ctx;
	if (shm_recv_ctx && shm_recv_ctx->shm_stat == NTM_SHM_READY) {
		ntm_shm_close(shm_recv_ctx);
		ntm_shm_destroy(shm_recv_ctx);
	}

	DEBUG("Destroy the ntm-nts context resources pass");

//	nts_shm_context_t shm_send_ctx;
//	shm_send_ctx = ntm_mgr->nts_ctx->shm_send_ctx;
//	if (shm_send_ctx && shm_send_ctx->shm_stat == NTS_SHM_READY) {
//		nts_shm_ntm_close(shm_send_ctx);
//		nts_shm_destroy(shm_send_ctx);
//	}


	/**
	 * Destroy the context structure
	 */
	free(ntm_mgr->ntm_conn_ctx);
	free(ntm_mgr->nt_listener_ctx);
	free(ntm_mgr->ntp_ctx);
	free(ntm_mgr->nts_ctx);

	free(ntm_mgr);

	DEBUG("ntm_destroy pass");

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

/*----------------------------------------------------------------------------*/
/**
 * the methods which are used to handle requests from remote ntb-monitor
 */
static inline void send_connect_ok_msg(ntm_conn_t ntm_conn);

inline void send_connect_ok_msg(ntm_conn_t ntm_conn) {
	assert(ntm_conn);

	ntm_sock_msg outgoing_msg;
	outgoing_msg.type = CONNECT_OK;
	ntm_send_tcp_msg(ntm_conn->client_sock, ((char *) &outgoing_msg),
			sizeof(outgoing_msg));

	DEBUG("send CONNECT_OK message pass");

}

void * ntm_sock_listen_thread(void *args) {
	assert(ntm_mgr);
	assert(ntm_mgr->ntm_conn_ctx);

	DEBUG(
			"local ntb-monitor ready to receive the connection request from remote ntb-monitor.");
	ntm_conn_ctx_t ntm_conn_ctx;
	ntm_conn_ctx = ntm_mgr->ntm_conn_ctx;

	ntm_socket_t client_sock;
	char *tmp_ip;
	ntm_conn_ctx->stop_signal = 0; // if 0, running; else stop

	// begin listening
	ntm_start_tcp_server(ntm_conn_ctx->server_sock, ntm_conn_ctx->listen_port,
			ntm_conn_ctx->listen_ip);
	DEBUG("ntm listen socket start listening...");

	while (!ntm_conn_ctx->stop_signal) {
		DEBUG("ready to accept connection");

		sleep(3);
		break;

		// accept connection
		client_sock = ntm_accept_tcp_conn(ntm_conn_ctx->server_sock);
		if(!client_sock) {
			if (!ntm_conn_ctx->stop_signal) {
				break;
			}
			continue;
		}

		// start async recv thread to receive messages
		struct ntm_conn client_conn;
		client_conn.stop_signal = false;
		client_conn.client_sock = client_sock;
		client_conn.sockfd = client_sock->socket_fd;
		client_conn.port = ntohs(client_sock->remote.sin_port);
		tmp_ip = inet_ntoa(client_sock->remote.sin_addr);
		client_conn.addrlen = sizeof(tmp_ip);
		memcpy(client_conn.ip, tmp_ip, sizeof(tmp_ip) + 1);

		DEBUG("accept connection from %s:%d", tmp_ip, client_conn.port);

		// Push the new client connection into hash map
		// key: ip from remote ntb-monitor
		// value: struct ntm_conn
		Put(ntm_conn_ctx->conn_map, tmp_ip, &client_conn);

		pthread_create(&client_conn.recv_thr, NULL, ntm_sock_recv_thread,
				&client_conn);

		send_connect_ok_msg(&client_conn);

	}

	DEBUG(
			"local ntb-monitor end the listening thread to stop receive connection");

	return NULL;
}

void * ntm_sock_recv_thread(void *args) {
	assert(ntm_mgr);
	assert(args);

	DEBUG("new ntm_sock receive thread start...");

	ntm_conn_t ntm_conn;
	ntm_conn = (ntm_conn_t) args;

	while (!ntm_conn->stop_signal) {
		ntm_sock_msg incoming_msg;
		ntm_recv_tcp_msg(ntm_conn->client_sock, (char *) &incoming_msg,
				sizeof(incoming_msg));
		DEBUG("recv a message");
		ntm_sock_handle_msg(ntm_conn, incoming_msg);

	}

	DEBUG("new ntm_sock receive thread end!!!");

	return NULL;
}

void ntm_sock_handle_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {
	assert(ntm_conn);
	assert(ntm_conn->client_sock);

	switch (msg.type) {
	case TRACK: {
		// TRACK code block
		printf("TRACK message \n");
		break;
	}
	case STOP: {
		// STOP code block
		printf("STOP message \n");
		break;
	}
	case SUCCESS: {
		human_data *data = (human_data *) &msg.payload;
		printf("SUCCESS message \n");
		break;
	}
	case TRACK_CONFIRM: {
		// code block
		printf("TRACK_CONFIRM message \n");
		break;
	}
	case CONNECT_OK: {
		// code block
		printf("CONNECT_OK message \n");
		break;
	}
	case STOP_CONFIRM: {
		// code block
		printf("STOP_CONFIRM message \n");
		break;
	}
	case FAILURE: {
		// code block
		printf("FAILURE message \n");
		break;
	}
	case QUIT: {
		// code block
		printf("QUIT message \n");
		break;
	}

	// add more cases s based on the number of types to be received

	}

}

void * nts_shm_recv_thread(void *args) {
	assert(ntm_mgr);
	assert(ntm_mgr->nts_ctx->shm_recv_ctx);

	DEBUG("nts_recv_thread ready...");
	ntm_nts_context_t nts_ctx;
	nts_ctx = ntm_mgr->nts_ctx;

	int retval;
	ntm_msg recv_msg;
	nts_ctx->shm_recv_signal = 1; // if 1, running; else stop

	while (nts_ctx->shm_recv_signal) {
		retval = ntm_shm_recv(nts_ctx->shm_recv_ctx, &recv_msg);
		if (retval == 0) {
			DEBUG("receive a message");
			nts_shm_handle_msg(ntm_mgr, recv_msg);

		} else {
			break;
		}

	}

	DEBUG("nts_recv_thread end!");

	return NULL;
}

void nts_shm_handle_msg(ntm_manager_t ntm_mgr, ntm_msg msg) {
	assert(ntm_mgr);

	DEBUG("nts shm handler function");

	if (msg.msg_type & NTM_MSG_INIT) {
		DEBUG("handle NTM_MSG_INIT");

	} else if (msg.msg_type & NTM_MSG_NEW_SOCK) {
		DEBUG("handle NTM_MSG_NEW_SOCK");

	} else if (msg.msg_type & NTM_MSG_BIND) {
		DEBUG("handle NTM_MSG_BIND");

	} else if (msg.msg_type & NTM_MSG_LISTEN) {
		DEBUG("handle NTM_MSG_LISTEN");

	} else if (msg.msg_type & NTM_MSG_ACCEPT) {
		DEBUG("handle NTM_MSG_ACCEPT");

	} else if (msg.msg_type & NTM_MSG_CONNECT) {
		DEBUG("handle NTM_MSG_CONNECT");

	} else if (msg.msg_type & NTM_MSG_ESTABLISH) {
		DEBUG("handle NTM_MSG_ESTABLISH");

	} else if (msg.msg_type & NTM_MSG_CLOSE) {
		DEBUG("handle NTM_MSG_CLOSE");

	} else if (msg.msg_type & NTM_MSG_DISCONNECT) {
		DEBUG("handle NTM_MSG_DISCONNECT");

	} else {
		DEBUG("Error NTM Message Type");

	}

}

int print_monitor() {
	printf("Hello Ntb Monitor.\n");

//	test_ntm_ring();
//	test_ntm_shm();
//	test_nts_shm();
	printf("Bye, Ntb Monitor.\n");

	return 0;
}
