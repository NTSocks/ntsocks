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
#include <unistd.h>

#include "ntb_monitor.h"
#include "config.h"
#include "ntm_shmring.h"
#include "ntm_shm.h"
#include "nt_log.h"
#include "nt_errno.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define MSG "Hello NT-Monitor! I am libnts app. Nice to meet you!"

static void *nts_recv_thread(void *arg);

static void *nts_send_thread(void *arg);

static void *ntp_recv_thread(void *arg);

static void *ntp_send_thread(void *arg);

static inline bool try_close_ntm_listener(ntm_conn_ctx_t ntm_conn_ctx);

/**
 * destroy nt_socket-level nt_socket-related resources, i.e., nts_shm_conn, nts_shm_ctx, nt_socket_id, nt_port
 * 		nts_shm_context, accepted_conn_map, port_sock_map, nts_shm_conn_map(ntm_nts_context)
 */
static inline void destroy_client_nt_socket_conn(nt_socket_t client_socket, nts_shm_conn_t nts_shm_conn, int bound_port);
static inline void destroy_client_nt_socket_conn_with_sock(nt_socket_t client_socket);
static inline void destroy_client_nt_socket_conn_with_nts_shm_conn(nts_shm_conn_t nts_shm_conn);

static inline void test_ntm_ring()
{
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

static inline void test_ntm_shm()
{
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

static inline void test_nts_shm()
{
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

void *nts_recv_thread(void *arg)
{
	assert(ntm_mgr->nts_ctx->shm_recv_ctx);

	// ntm_nts_context_t nts_ctx;
	// nts_ctx = ntm_mgr->nts_ctx;

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

void *nts_send_thread(void *arg)
{

	DEBUG("ntm_sock_recv_thread ready...");

	DEBUG("ntm_sock_recv_thread end!");

	return 0;
}

void *ntp_recv_thread(void *arg)
{

	return 0;
}

void *ntp_send_thread(void *arg)
{

	return 0;
}

ntm_manager_t get_ntm_manager()
{
	assert(ntm_mgr);
	return ntm_mgr;
}

int ntm_init(const char *config_file)
{

	/**
	 * read conf file and init ntm params
	 */
	DEBUG("load the ntm config file");
	load_conf(config_file);

	ntm_nts_context_t nts_ctx;
	ntm_ntp_context_t ntp_ctx;
	ntm_conn_ctx_t ntm_conn_ctx;
	nt_listener_context_t nt_listener_ctx;
	int ret;

	ntm_mgr = (ntm_manager_t)calloc(1, sizeof(struct ntm_manager));
	const char *err_msg = "malloc";
	if (!ntm_mgr)
	{
		perror(err_msg);
		ERR("Failed to allocate ntm_manager.");
		goto FAIL;
	}

	// hash map for the mapping relationship between port and client nt_socket
	ntm_mgr->port_sock_map = createHashMap(NULL, NULL);


	ntm_mgr->nts_ctx = (ntm_nts_context_t)calloc(1,
												 sizeof(struct ntm_nts_context));
	if (!ntm_mgr->nts_ctx)
	{
		perror(err_msg);
		ERR("Failed to allocate ntm_nts_context.");
		goto FAIL;
	}
	nts_ctx = ntm_mgr->nts_ctx;
	// hash map for cache the nts shm ringbuffer context created by nts
	nts_ctx->nts_shm_conn_map = createHashMap(NULL, NULL);

	ntm_mgr->ntp_ctx = (ntm_ntp_context_t)calloc(1,
												 sizeof(struct ntm_ntp_context));
	if (!ntm_mgr->ntp_ctx)
	{
		perror(err_msg);
		ERR("Failed to allocate ntm_ntp_context.");
		goto FAIL;
	}
	ntp_ctx = ntm_mgr->ntp_ctx;


	ntm_mgr->ntm_conn_ctx = (ntm_conn_ctx_t)calloc(1,
												   sizeof(struct ntm_conn_context));
	if (!ntm_mgr->ntm_conn_ctx)
	{
		perror(err_msg);
		ERR("Failed to allocate ntm_ntp_context.");
		goto FAIL;
	}
	ntm_conn_ctx = ntm_mgr->ntm_conn_ctx;
	// hash map for cache the remote ntm connection list
	ntm_conn_ctx->conn_map = createHashMap(NULL, NULL);


	ntm_mgr->nt_listener_ctx = (nt_listener_context_t)calloc(1,
															 sizeof(struct nt_listener_context));
	if (!ntm_mgr->nt_listener_ctx)
	{
		perror(err_msg);
		ERR("Failed to allocate ntm_ntp_context.");
		goto FAIL;
	}
	nt_listener_ctx = ntm_mgr->nt_listener_ctx;
	// hash map for holding all nt_listener indexed by port
	nt_listener_ctx->listener_map = createHashMap(NULL, NULL);



	/**
	 *	init global socket and port resources
	 *	TODO:
	 */
	DEBUG("Init global socket and port resources");
	ntm_mgr->nt_sock_ctx = (nt_sock_context_t) calloc (1, sizeof(struct nt_sock_context));
	if(!ntm_mgr->nt_sock_ctx) {
		perror(err_msg);
		ERR("Failed to allocate sock_context.");
		goto FAIL;
	}
	init_socket_context(ntm_mgr->nt_sock_ctx, NTM_CONFIG.max_concurrency);

	ntm_mgr->nt_port_ctx = (nt_port_context_t) calloc (1, sizeof(struct nt_port_context));
	if(!ntm_mgr->nt_port_ctx) {
		perror(err_msg);
		ERR("Failed to allocate port_context");
		goto FAIL;
	}
	init_port_context(ntm_mgr->nt_port_ctx, NTM_CONFIG.max_port);



	/**
	 * init the ntp-related shm ringbuffer queue to recv/send from/to ntb-proxy
	 * init the ntp ==> ntm shm recv queue
	 * init the ntm ==> ntp shm send queue
	 */
	// init the ntp ==> ntm shm recv queue
	ntp_ctx->shm_recv_ctx = ntp_ntm_shm();
	if (!ntp_ctx->shm_recv_ctx) {
		ERR("Failed to allocate ntp_ntm_shm_context.");
		goto FAIL;
	}

	ret = ntp_ntm_shm_connect(ntp_ctx->shm_recv_ctx, 
				NTP_NTM_SHM_NAME, sizeof(NTP_NTM_SHM_NAME));
	if (ret == -1) {
		ERR("ntp_ntm_shm_connect failed.");
		goto FAIL;
	}

	// init the ntm ==> ntp shm send queue
	ntp_ctx->shm_send_ctx = ntm_ntp_shm();
	if (!ntp_ctx->shm_send_ctx) {
		ERR("Failed to allocate ntp_ntm_shm_context.");
		goto FAIL;
	}

	ret = ntm_ntp_shm_connect(ntp_ctx->shm_send_ctx, 
				NTM_NTP_SHM_NAME, sizeof(NTM_NTP_SHM_NAME));
	if (ret == -1) {
		ERR("ntm_ntp_shm_connect failed.");
		goto FAIL;
	}



	/**
	 * init the ntm shm ringbuffer to receive the messages from libnts apps
	 */
	ntm_mgr->nts_ctx->shm_recv_ctx = ntm_shm();
	if (!ntm_mgr->nts_ctx->shm_recv_ctx)
	{
		ERR("Failed to allocate ntm_shm_context.");
		goto FAIL;
	}

	ret = ntm_shm_accept(ntm_mgr->nts_ctx->shm_recv_ctx,
						 NTM_SHMRING_NAME, sizeof(NTM_SHMRING_NAME));
	if (ret)
	{
		ERR("ntm_shm_accept failed.");
		goto FAIL;
	}

	// pthread_create(&nts_ctx->shm_recv_thr, NULL, nts_recv_thread, NULL);

	/**
	 * init the listen socket for the connection requests from remote nt-monitor
	 */
	ntm_conn_ctx->listen_ip = NTM_CONFIG.listen_ip;
	ntm_conn_ctx->listen_port = NTM_CONFIG.listen_port;
	ntm_conn_ctx->addrlen = NTM_CONFIG.ipaddr_len;
	ntm_conn_ctx->running_signal = 0;
	ntm_conn_ctx->server_sock = ntm_sock_create();
	pthread_create(&ntm_conn_ctx->ntm_sock_listen_thr, NULL,
				   	ntm_sock_listen_thread, ntm_conn_ctx);
	//	pthread_create(&(ntm_conn_ctx->ntm_sock_listen_thr), NULL,
	//			nts_send_thread, NULL);

	DEBUG("ntm_init pass");

	nts_shm_recv_thread(NULL);

	return 0;

	FAIL:

	return -1;

}

void ntm_destroy()
{
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
	while (hasNextHashMapIterator(iter))
	{
		DEBUG("iter next");
		iter = nextHashMapIterator(iter);
		ntm_conn = iter->entry->value;
		ntm_close_socket(ntm_conn->client_sock);
		Remove(ntm_mgr->ntm_conn_ctx->conn_map, ntm_conn->ip);
	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->ntm_conn_ctx->conn_map);
	DEBUG("free hashmap for ntm_conn context pass");

	if (ntm_mgr->ntm_conn_ctx->server_sock)
	{
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
	nt_listener_wrapper_t nt_listener_wrapper;
	nt_socket_t tmp_socket;
	iter = createHashMapIterator(ntm_mgr->nt_listener_ctx->listener_map);
	while (hasNextHashMapIterator(iter))
	{
		iter = nextHashMapIterator(iter);
		nt_listener_wrapper = iter->entry->value;

		nt_conn_iter = createHashMapIterator(nt_listener_wrapper->accepted_conn_map);

		while (hasNextHashMapIterator(nt_conn_iter))
		{
			nt_conn_iter = nextHashMapIterator(iter);
			tmp_socket = (nt_socket_t) nt_conn_iter->entry->value;

			Remove(nt_listener_wrapper->accepted_conn_map, &tmp_socket->sockid);
		}
		freeHashMapIterator(&nt_conn_iter);
		Clear(nt_listener_wrapper->accepted_conn_map);
		DEBUG("free hash map for client socket connection accepted by the specified nt_listener socket pass");

		// free or unbound nt_listener socket id
		Remove(ntm_mgr->nt_listener_ctx->listener_map, &nt_listener_wrapper->port);
		free_socket(ntm_mgr->nt_sock_ctx, nt_listener_wrapper->listener->sockid, 1);

	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->nt_listener_ctx->listener_map);
	DEBUG("Destroy the nt_listener related resources pass");

	/**
	 * Destroy the ntm-nts context resources
	 */
	nts_shm_conn_t nts_shm_conn;
	iter = createHashMapIterator(ntm_mgr->nts_ctx->nts_shm_conn_map);
	while (hasNextHashMapIterator(iter))
	{
		iter = nextHashMapIterator(iter);
		nts_shm_conn = iter->entry->value;

		// free or unbound socket id
		free_socket(ntm_mgr->nt_sock_ctx, nts_shm_conn->sockid, 1);

		// free nts_shm_context
		if (nts_shm_conn->nts_shm_ctx && nts_shm_conn->nts_shm_ctx->shm_stat == NTS_SHM_READY)
		{
			nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
			nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
		}

		Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, &nts_shm_conn->sockid);
	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->nts_ctx->nts_shm_conn_map);
	DEBUG("free hash map for nts_shm_conn pass");

	ntm_shm_context_t shm_recv_ctx;
	shm_recv_ctx = ntm_mgr->nts_ctx->shm_recv_ctx;
	if (shm_recv_ctx && shm_recv_ctx->shm_stat == NTM_SHM_READY)
	{
		ntm_shm_close(shm_recv_ctx);
		ntm_shm_destroy(shm_recv_ctx);
		shm_recv_ctx = NULL;
	}

	DEBUG("Destroy the ntm-nts context resources pass");

	// Destroy the port_sock_map hashmap
	Clear(ntm_mgr->port_sock_map);

	/**
	 * Destroy the ntp-related (ntp <==> ntm) resources/context, including:
	 * 1. destroy ntp ==> ntm shm recv queue
	 * 2. destroy ntm ==> ntp shm send queue
	 */
	ntm_ntp_context_t ntp_ctx;
	ntp_ctx = ntm_mgr->ntp_ctx;
	if (ntp_ctx->shm_send_ctx && 
				ntp_ctx->shm_send_ctx->shm_stat == SHM_READY) {
		ntm_ntp_shm_ntm_close(ntp_ctx->shm_send_ctx);
		ntm_ntp_shm_destroy(ntp_ctx->shm_send_ctx);
		ntp_ctx->shm_send_ctx = NULL;
	}
	if (ntp_ctx->shm_recv_ctx &&
				ntp_ctx->shm_recv_ctx->shm_stat == SHM_READY) {
		ntp_ntm_shm_ntm_close(ntp_ctx->shm_recv_ctx);
		ntp_ntm_shm_destroy(ntp_ctx->shm_recv_ctx);
		ntp_ctx->shm_recv_ctx = NULL;
	}

	DEBUG("Destroy the ntm-ntp context resources pass");



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
	ntm_mgr->ntm_conn_ctx = NULL;
	free(ntm_mgr->nt_listener_ctx);
	ntm_mgr->nt_listener_ctx = NULL;
	free(ntm_mgr->ntp_ctx);
	ntm_mgr->ntp_ctx = NULL;
	free(ntm_mgr->nts_ctx);
	ntm_mgr->nts_ctx = NULL;

	free(ntm_mgr);
	ntm_mgr = NULL;

	/**
     * destroy the memory which is allocated to NTM_CONFIG
     */
    free_conf();

	DEBUG("ntm_destroy pass");
}

int ntm_getconf(struct ntm_config *conf)
{

	return 0;
}

int ntm_setconf(const struct ntm_config *conf)
{

	return 0;
}

ntm_manager_t init_ntm_manager()
{

	return NULL;
}

/*----------------------------------------------------------------------------*/
/**
 * the methods which are used to handle requests from local/remote ntb-monitor
 */
static inline void send_connect_ok_msg(ntm_conn_t ntm_conn);

static inline void handle_nt_syn_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_nt_syn_ack_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_nt_invalid_port_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_nt_listener_not_found_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_nt_listener_not_ready_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_backlog_is_full_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);

static inline void handle_nt_fin_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_nt_fin_ack_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);

static inline void handle_connect_ok_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_failure_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_stop_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
static inline void handle_stop_confirm_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);


inline void handle_nt_syn_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	ntm_sock_msg outgoing_msg;
	outgoing_msg.src_addr = msg.dst_addr;
	outgoing_msg.sport = msg.dport;
	outgoing_msg.dport = msg.sport;
	outgoing_msg.dst_addr = msg.src_addr;

	// check whether the SYN message is valid or not
	int sport, dport;
	sport = ntohs(msg.sport);
	dport = ntohs(msg.dport);
	if (sport <=0 || dport <=0)
	{
		outgoing_msg.type = NT_INVALID_PORT;
		ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		return;
	}
	
	// check whether the specified dport has conresponding nt_listener
	if(!Exists(ntm_mgr->nt_listener_ctx->listener_map, &dport)){
		outgoing_msg.type = NT_LISTENER_NOT_FOUND;
		ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		return;
	}

	nt_listener_wrapper_t listener_wrapper;
	listener_wrapper = (nt_listener_wrapper_t)Get(ntm_mgr->nt_listener_ctx->listener_map, &dport);
	if (listener_wrapper->listener->socket->state != LISTENING)
	{
		outgoing_msg.type = NT_LISTENER_NOT_READY;
		ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		return;
	}

	if (backlog_is_full(listener_wrapper->backlog_ctx))
	{
		outgoing_msg.type = NT_BACKLOG_IS_FULL;
		ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		return;
	}
	
	/**
	 * If valid, 
	 * 	(1)	generate client_socket to setup and push into backlog,
	 * 		the socket type is setted as `NT_SOCK_PIPE` for accepted nt_socket
	 */
	nt_socket_t client_socket;
	client_socket = allocate_socket(ntm_mgr->nt_sock_ctx, NT_SOCK_PIPE, 1);
	client_socket->listener = listener_wrapper->listener;
	bzero(&client_socket->saddr, sizeof(struct sockaddr_in));
	client_socket->saddr.sin_family = AF_INET;
	client_socket->saddr.sin_port = msg.dport;
	client_socket->saddr.sin_addr.s_addr = msg.dst_addr;
	// allocate a idle port for the accepted client socket
	nt_port_t port;
	port = allocate_port(ntm_mgr->nt_port_ctx, 1);
	client_socket->saddr.sin_port = htons(port->port_id);
	// cache the <port, nt_socket> mapping in `ntm_mgr->port_sock_map`
	Put(ntm_mgr->port_sock_map, &port->port_id, client_socket);


	// TODO:
	// instruct ntb-proxy to setup the ntb connection/queue
	// i. setup ntp msg and send to ntp shm
	// ii. wait for the response msg from ntp
	ntm_ntp_msg ntp_outgoing_msg;	// for send msg into ntp
	
	ntp_outgoing_msg.dst_ip = msg.src_addr;
	ntp_outgoing_msg.dst_port = msg.sport;
	ntp_outgoing_msg.src_ip = msg.dst_addr;
	ntp_outgoing_msg.src_port = msg.dport;
	ntp_outgoing_msg.msg_type = 1;
	ntp_outgoing_msg.msg_len = 0;
	int retval;
	retval = ntm_ntp_shm_send(ntm_mgr->ntp_ctx->shm_send_ctx, &ntp_outgoing_msg);
	while (retval == -1) {
		sched_yield();
		retval = ntm_ntp_shm_send(ntm_mgr->ntp_ctx->shm_send_ctx, &ntp_outgoing_msg);
	}
	DEBUG("ntm_ntp_shm_send pass, wait to recv response from ntp.");

	ntp_ntm_msg ntp_incoming_msg;	// for recv msg from ntp
	retval = ntp_ntm_shm_recv(ntm_mgr->ntp_ctx->shm_recv_ctx, &ntp_incoming_msg);
	while (retval == -1) {
		sched_yield();
		retval = ntp_ntm_shm_recv(ntm_mgr->ntp_ctx->shm_recv_ctx, &ntp_incoming_msg);
	}
	
	// if ntb connection setup failed, goto FAIL.
	if (ntp_incoming_msg.msg_type != 1) {
		ERR("ntb connection setup failed");
		goto FAIL;
	}



	DEBUG("recv the response message of `ntb connection setup successfully` from ntp.");

	/** 
	 * TODO: if receive `SETUP Success` message from ntb-proxy (ntp_incoming_msg.msg_type == 1),
	 * 	init/create coresponding `nts_shm_conn_t` for the accepted nt_socket,
	 * 	push new nts shm conn into hashmap,
	 * 	update client socket state as `ESTABLISHED`
	 */
	
	// init/create coresponding `nts_shm_conn_t` for the accepted nt_socket
	nts_shm_conn_t new_nts_shm_conn;
	new_nts_shm_conn = (nts_shm_conn_t) calloc(1, sizeof(struct nts_shm_conn));
	if (!new_nts_shm_conn) {
		perror(MALLOC_ERR_MSG);
		ERR("calloc new_nts_shm_conn failed");
		goto FAIL;
	}
	new_nts_shm_conn->socket = client_socket;
	new_nts_shm_conn->sockid = new_nts_shm_conn->socket->sockid;
	new_nts_shm_conn->running_signal = 1;
	new_nts_shm_conn->listener = listener_wrapper;

	// push new nts shm conn into hashmap
	Put(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid, new_nts_shm_conn);

	// update client socket state as `ESTABLISHED`
	client_socket->state = ESTABLISHED;

	// push the new `client_socket` into coresponding `listener_wrapper->accepted_conn_map`
	Put(listener_wrapper->accepted_conn_map, &client_socket->sockid, client_socket);

	backlog_push(listener_wrapper->backlog_ctx, client_socket);


	// send `SYN_ACK` back to source monitor.
	outgoing_msg.type = NT_SYN_ACK;
	ntm_send_tcp_msg(ntm_conn->client_sock, 
				(char*)&outgoing_msg, sizeof(outgoing_msg));

	DEBUG("handle NT_SYN message pass");

	FAIL: 
	if (port) {
		if (Exists(ntm_mgr->port_sock_map, &port->port_id)) {
			Remove(ntm_mgr->port_sock_map, &port->port_id);
		}

		free_port(ntm_mgr->nt_port_ctx, port->port_id, 1);
	}

	if(client_socket) {
		client_socket->state = CLOSED;

		if (Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid)) {
			Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid);
		}

		if (listener_wrapper && 
				Exists(listener_wrapper->accepted_conn_map, &client_socket->sockid)) {
			Remove(listener_wrapper->accepted_conn_map, &client_socket->sockid);
		}

		free_socket(ntm_mgr->nt_sock_ctx, client_socket->sockid, 1);
	}

	if (new_nts_shm_conn != NULL) {
		free(new_nts_shm_conn);
	}

	return;
}

inline void handle_nt_syn_ack_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {
	int sport, dport;
	sport = ntohs(msg.sport);
	dport = ntohs(msg.dport);

	// check whether the SYN_ACK message is valid or not
	if (sport <= 0 || dport <= 0) {
		ERR("Invalid port value.");
		return;
	}

	// locate the coressponding client nt_client via `dport`
	nt_socket_t client_sock;
	client_sock = (nt_socket_t) Get(ntm_mgr->port_sock_map, &dport);
	if (!client_sock) {
		ERR("Invalid or Non-existing destination port.");
		return;
	}

	/**
	 * receive `NT_SYN_ACK`, then tell ntp to setup ntb connection,
	 * i. setup ntp msg and send to ntp shm
	 * ii. wait for the response msg from ntp
	 *  update the nt_socket state as `ESTABLISHED`. 
	 */
	ntm_ntp_msg ntp_outgoing_msg;	// for send msg into ntp
	ntp_outgoing_msg.dst_ip = msg.src_addr;
	ntp_outgoing_msg.dst_port = msg.sport;
	ntp_outgoing_msg.src_ip = msg.dst_addr;
	ntp_outgoing_msg.src_port = msg.dport;
	ntp_outgoing_msg.msg_type = 1;
	ntp_outgoing_msg.msg_len = 0;
	int retval;
	retval = ntm_ntp_shm_send(ntm_mgr->ntp_ctx->shm_send_ctx, &ntp_outgoing_msg);
	while (retval == -1) {
		sched_yield();
		retval = ntm_ntp_shm_send(ntm_mgr->ntp_ctx->shm_send_ctx, &ntp_outgoing_msg);
	}
	DEBUG("ntm_ntp_shm_send pass, wait to recv response from ntp.");

	ntp_ntm_msg ntp_incoming_msg;	// for recv msg from ntp
	retval = ntp_ntm_shm_recv(ntm_mgr->ntp_ctx->shm_recv_ctx, &ntp_incoming_msg);
	while (retval == -1) {
		sched_yield();
		retval = ntp_ntm_shm_recv(ntm_mgr->ntp_ctx->shm_recv_ctx, &ntp_incoming_msg);
	}
	
	// if ntb connection setup failed, goto FAIL.
	if (ntp_incoming_msg.msg_type != 1) {
		ERR("ntb connection setup failed");
		goto FAIL;
	}

	DEBUG("recv the response message of `ntb connection setup successfully` from ntp.");

	// update the nt_socket state as `ESTABLISHED`
	client_sock->state = ESTABLISHED;


	if (!Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid))
	{
		ERR("The nts_shm_conn of client socket[sockid=%d, port=%d] is not found", (int)client_sock->sockid, dport);
		return;
	}
	nts_shm_conn_t nts_shm_conn;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid);


	nts_msg response_msg;
	response_msg.msg_type = NTS_MSG_ESTABLISH;
	response_msg.sockid = client_sock->sockid;
	response_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if(retval) {
		ERR("nts_shm_send failed for response to NTS_MSG_ESTABLISH");
		return;
	}

	DEBUG("handle NT_SYN_ACK message pass");

	FAIL: 

	return;
}

inline void handle_nt_invalid_port_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_INVALID_PORT message pass");
}

 
inline void handle_nt_listener_not_found_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {


	DEBUG("handle NT_LISTENER_NOT_FOUND message pass");
}

inline void handle_nt_listener_not_ready_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_LISTENER_NOT_READY message pass");
}

inline void handle_backlog_is_full_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_BACKLOG_IS_FULL pass");
}


inline void handle_nt_fin_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	


	DEBUG("handle NT_FIN pass");
}

inline void handle_nt_fin_ack_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	/**
	 * 1. locate the local coressponding nt_socket in `ntm_mgr->port_sock_map` according to the `<dip, dport, sip, sport>` 
	 * 2. get `nts_shm_conn_t` according to nt_socket, update nt_socket state to `WAIT_CLOSE`
	 * 3. nts_shm_send() `NTS_MSG_CLOSE` nts_msg to libnts
	 * 4. destroy nt_socket-related resources, i.e., nts_shm_conn, nt_socket_id, 
	 * 		nts_shm_context, accepted_conn_map, port_sock_map
	 */
	int sport, dport;
	sport = ntohs(msg.sport);
	dport = ntohs(msg.dport);

	// check whether the FIN_ACK message is valid or not
	if (sport <= 0 || dport <= 0) {
		ERR("Invalid port value.");
		return;
	}

	// locate the coressponding client nt_client via `dport`
	nt_socket_t client_sock;
	client_sock = (nt_socket_t) Get(ntm_mgr->port_sock_map, &dport);
	if (!client_sock) {
		ERR("Invalid or Non-existing destination port.");
		return;
	}

	// update the nt_socket state as `WAIT_CLOSE`
	client_sock->state = WAIT_CLOSE;

	// judge whether the coresponding `nts_shm_conn` exists or not
	// if exists, get it
	if (!Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid))
	{
		ERR("The nts_shm_conn of client socket[sockid=%d, port=%d] is not found", (int)client_sock->sockid, dport);
		return;
	}
	nts_shm_conn_t nts_shm_conn;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid);


	/**
	 * 3. nts_shm_send() `NTS_MSG_CLOSE` nts_msg to libnts
	 */
	int retval;
	nts_msg response_msg;
	response_msg.msg_type = NTS_MSG_CLOSE;
	response_msg.sockid = client_sock->sockid;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if(retval) {
		ERR("nts_shm_send failed for response to NTS_MSG_ESTABLISH");
		return;
	}

	client_sock->state = CLOSED;	


	/**
	 * 4. destroy nt_socket-related resources, i.e., nts_shm_conn, nts_shm_ctx, nt_socket_id, nt_port
	 * 		nts_shm_context, accepted_conn_map, port_sock_map, nts_shm_conn_map(ntm_nts_context)
	 */
	destroy_client_nt_socket_conn(client_sock, nts_shm_conn, dport);
	
	// // Remove `client_socket` from `ntm_mgr->port_sock_map`
	// if (Exists(ntm_mgr->port_sock_map, &dport))
	// 	Remove(ntm_mgr->port_sock_map, &dport);

	// // free nt_port 
	// free_port(ntm_mgr->nt_port_ctx, dport, 1);

	// // if accepted client socket (created by `accept()`),
	// // then Remove coresponding `nt_socket_t` in `accepted_conn_map` of `nt_listener_wrapper`
	// if (client_sock->socktype == NT_SOCK_PIPE) {
	// 	if (nts_shm_conn->listener && 
	// 			Exists(nts_shm_conn->listener->accepted_conn_map, &client_sock->sockid)) {
	// 		Remove(nts_shm_conn->listener->accepted_conn_map, &client_sock->sockid);
	// 	}
	// }

	// // close and destroy nts_shm_context_t in `nts_shm_conn`
	// // free nts_shm_conn
	// if(nts_shm_conn) {
	// 	if (nts_shm_conn->nts_shm_ctx) {
	// 		nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
	// 		nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
	// 	}

	// 	if (Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid)) {
	// 		Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid);
	// 	}
	// 	free(nts_shm_conn);

	// }
	
	// if (client_sock) {
	// 	free_socket(ntm_mgr->nt_sock_ctx, client_sock->sockid, 1);
	// }


	DEBUG("handle NT_FIN_ACK pass");
}



inline void handle_connect_ok_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

}

inline void handle_failure_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

}

inline void handle_stop_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	/**
	 * When ntm_conn stop: 
	 *  1. Update running_signal as `false`
	 *  2. Send `STOP_CONFIRM` message to remote ntm
	 *  3. Wait a while and destory local ntm_conn
	 */
	ntm_conn->running_signal = false;

	int retval;
	ntm_sock_msg outgoing_msg;
	outgoing_msg.type = STOP_CONFIRM;
	outgoing_msg.dst_addr = msg.src_addr;
	outgoing_msg.dport = msg.sport;
	outgoing_msg.src_addr = msg.dst_addr;
	outgoing_msg.sport = msg.dport;
	retval = ntm_send_tcp_msg(ntm_conn->client_sock, (char *)&outgoing_msg, sizeof(outgoing_msg));
	if(retval <= 0) {
		ERR("ntm_send_tcp_msg send STOP_CONFIRM message failed");
		return;
	}

	// wait a while and destroy local ntm_conn
	usleep(1000);
	ntm_close_socket(ntm_conn->client_sock);

}

inline void handle_stop_confirm_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	/**
	 * When recv `STOP_CONFIRM`:
	 *  1. running_signal as `false`
	 *  2. Wait a while and destroy local ntm_conn
	 */
	ntm_conn->running_signal = false;

	usleep(1000);
	ntm_close_socket(ntm_conn->client_sock);

}


inline void send_connect_ok_msg(ntm_conn_t ntm_conn)
{
	assert(ntm_conn);

	ntm_sock_msg outgoing_msg;
	outgoing_msg.type = CONNECT_OK;
	ntm_send_tcp_msg(ntm_conn->client_sock, ((char *)&outgoing_msg),
					 sizeof(outgoing_msg));

	DEBUG("send CONNECT_OK message pass");
}

void *ntm_sock_listen_thread(void *args)
{
	assert(ntm_mgr);
	assert(ntm_mgr->ntm_conn_ctx);

	DEBUG(
		"local ntb-monitor ready to receive the connection request from remote ntb-monitor.");
	ntm_conn_ctx_t ntm_conn_ctx;
	ntm_conn_ctx = ntm_mgr->ntm_conn_ctx;

	ntm_socket_t client_sock;
	char *tmp_ip;
	ntm_conn_ctx->running_signal = 1;

	// begin listening
	ntm_start_tcp_server(ntm_conn_ctx->server_sock, ntm_conn_ctx->listen_port,
						 ntm_conn_ctx->listen_ip);
	DEBUG("ntm listen socket start listening...");

	while (ntm_conn_ctx->running_signal)
	{
		DEBUG("ready to accept connection");

		sleep(2);
		break;

		// accept connection
		client_sock = ntm_accept_tcp_conn(ntm_conn_ctx->server_sock);
		if (!client_sock)
		{
			if (!ntm_conn_ctx->running_signal)
			{
				break;
			}
			continue;
		}

		// start async recv thread to receive messages
		struct ntm_conn client_conn;
		client_conn.running_signal = true;
		client_conn.client_sock = client_sock;
		client_conn.sockfd = client_sock->socket_fd;
		client_conn.port = ntohs(client_sock->remote.sin_port);
		tmp_ip = inet_ntoa(client_sock->remote.sin_addr);
		client_conn.addrlen = strlen(tmp_ip);
		memcpy(client_conn.ip, tmp_ip, client_conn.addrlen);

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

void *ntm_sock_recv_thread(void *args)
{
	assert(ntm_mgr);
	assert(args);

	DEBUG("new ntm_sock receive thread start...");

	ntm_conn_t ntm_conn;
	ntm_conn = (ntm_conn_t)args;

	while (ntm_conn->running_signal)
	{
		ntm_sock_msg incoming_msg;
		ntm_recv_tcp_msg(ntm_conn->client_sock, (char *)&incoming_msg,
						 sizeof(incoming_msg));
		DEBUG("recv a message");
		ntm_sock_handle_msg(ntm_conn, incoming_msg);
	}

	DEBUG("new ntm_sock receive thread end!!!");

	return NULL;
}

void ntm_sock_handle_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg)
{
	assert(ntm_conn);
	assert(ntm_conn->client_sock);

	switch (msg.type)
	{
	case NT_SYN: {
		DEBUG("SYN message");

		/**
		 * Receive remote SYN message
		 * 1. check whether the SYN message is valid
		 * 2. If valid:
		 * 	i.   generate local client nt_socket
		 * 	ii.  instruct ntp to setup the ntb connection/queue
		 *  iii. update nt_socket state as `ESTABLISHED`, 
		 * 		push it into accepted_queue which will be polled by nts accept()
		 *  iv.  send `SYN_ACK` back to source monitor.
		 */
		handle_nt_syn_msg(ntm_conn, msg);

		break;
	}
	case NT_SYN_ACK: {
		DEBUG("SYN ACK message");
		
		/**
		 * Receive `SYN ACK` message from remote monitor
		 * 1. check whether the SYN ACK is valid
		 * 2. if valid:
		 * 	i. instruct ntp to setup the ntb connection/queue
		 *  ii. update nt_socket state as `ESTABLISHED`
		 *  iii. send `ESTABLISHED` message with 
		 * 		<shm name of send/recv queue> into nts process via nts shm queue.
		 */
		handle_nt_syn_ack_msg(ntm_conn, msg);

		break;
	}
	case NT_INVALID_PORT: {
		DEBUG("INVALID PORT message");

		handle_nt_invalid_port_msg(ntm_conn, msg);
		
		break;
	}
	case NT_LISTENER_NOT_FOUND: {
		DEBUG("NT LISTENER NOT FOUND message");

		handle_nt_listener_not_found_msg(ntm_conn, msg);

		break;
	}
	case NT_LISTENER_NOT_READY: {
		DEBUG("NT LISTENER NOT READY message");

		handle_nt_listener_not_ready_msg(ntm_conn, msg);

		break;
	}
	case NT_BACKLOG_IS_FULL: {
		DEBUG("NT BACKLOG IS FULL message");

		handle_backlog_is_full_msg(ntm_conn, msg);

		break;
	}
	case NT_FIN: {
		DEBUG("FIN message");

		handle_nt_fin_msg(ntm_conn, msg);

		break;
	}
	case NT_FIN_ACK: {
		DEBUG("FIN ACK message");

		handle_nt_fin_ack_msg(ntm_conn, msg);

		break;
	}
	case TRACK:
	{
		// TRACK code block
		DEBUG("TRACK message \n");
		break;
	}
	case STOP:
	{
		// STOP code block
		DEBUG("STOP message \n");

		handle_stop_msg(ntm_conn, msg);

		break;
	}
	case SUCCESS:
	{
		// human_data *data = (human_data *)&msg.payload;
		DEBUG("SUCCESS message \n");
		break;
	}
	case TRACK_CONFIRM:
	{
		// code block
		DEBUG("TRACK_CONFIRM message \n");
		break;
	}
	case CONNECT_OK:
	{
		// code block
		DEBUG("CONNECT_OK message \n");

		handle_connect_ok_msg(ntm_conn, msg);

		break;
	}
	case STOP_CONFIRM:
	{
		// code block
		DEBUG("STOP_CONFIRM message \n");
		handle_stop_confirm_msg(ntm_conn, msg);

		break;
	}
	case FAILURE:
	{
		// code block
		DEBUG("FAILURE message \n");

		handle_failure_msg(ntm_conn, msg);

		break;
	}
	case QUIT:
	{
		// code block
		DEBUG("QUIT message \n");
		break;
	}

		// add more cases s based on the number of types to be received
	}
}

/*----------------------------------------------------------------------------*/
/**
 * the methods which are used to handle requests from local nts apps, including:
 * 1. new socket
 * 2. bind
 * 3. listen
 * 4. connect
 * 5. accept
 * 6. close
 */
static inline void handle_msg_nts_new_socket(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_connect(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_bind(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_listen(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_accept(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_accept_ack(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_close(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_fin(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_err(ntm_manager_t ntm_mgr, ntm_msg msg);

inline void handle_msg_nts_new_socket(ntm_manager_t ntm_mgr, ntm_msg msg)
{
	// receive a nts shm ring buffer channel, create nts shm context to
	// send the response to nts app
	int retval;
	nts_shm_conn_t new_nts_shm_conn;
	new_nts_shm_conn = (nts_shm_conn_t) calloc(1, sizeof(struct nts_shm_conn));
	if (!new_nts_shm_conn) {
		perror(MALLOC_ERR_MSG);
		ERR("calloc new_nts_shm_conn failed");
		goto FAIL;
	}

	// allocate socket
	DEBUG("allocate free socket for new socket");
	new_nts_shm_conn->socket = allocate_socket(ntm_mgr->nt_sock_ctx, msg.sock_type, 1);
	new_nts_shm_conn->sockid = new_nts_shm_conn->socket->sockid;
	new_nts_shm_conn->domain = msg.domain;
	new_nts_shm_conn->protocol = msg.protocol;

	new_nts_shm_conn->running_signal = 1;
	// new_nts_shm_conn->is_listen = true;
	if (msg.nts_shm_addrlen > 0)
	{
		new_nts_shm_conn->shm_addrlen = msg.nts_shm_addrlen;
		memcpy(new_nts_shm_conn->nts_shm_name, msg.nts_shm_name, msg.nts_shm_addrlen);
	}

	new_nts_shm_conn->nts_shm_ctx = nts_shm();
	retval = nts_shm_connect(new_nts_shm_conn->nts_shm_ctx, msg.nts_shm_name, msg.nts_shm_addrlen);
	if (retval)
	{
		ERR("new nts_shm_conn connect nts shm ringbuffer failed");
		return;
	}

	// push new nts shm conn into hashmap
	Put(ntm_mgr->nts_ctx->nts_shm_conn_map, &new_nts_shm_conn->socket->sockid, new_nts_shm_conn);

	/**
	 * Send back response msg to nts app
	 */
	nts_msg response_msg;
	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = NTS_MSG_NEW_SOCK;
	response_msg.sockid = new_nts_shm_conn->sockid;
	retval = nts_shm_send(new_nts_shm_conn->nts_shm_ctx, &response_msg);
	if (retval)
	{
		ERR("nts_shm_send failed for response to NTM_MSG_NEW_SOCK");
		return;
	}

	DEBUG("handle NTM_MSG_NEW_SOCK pass");

	FAIL:
	if (new_nts_shm_conn) {
		if (new_nts_shm_conn->socket) {
			free_socket(ntm_mgr->nt_sock_ctx, new_nts_shm_conn->socket->sockid, 1);
		}

		if (new_nts_shm_conn->nts_shm_ctx) {
			nts_shm_ntm_close(new_nts_shm_conn->nts_shm_ctx);
			nts_shm_destroy(new_nts_shm_conn->nts_shm_ctx);
		}

		free(new_nts_shm_conn);
	}

	return;

}

/**
 * Responsible for:
 * 1. dispatch the `SYN` message to remote monitor 
 * 2. send `dispatched` message back to related nts app
 */
inline void handle_msg_nts_connect(ntm_manager_t ntm_mgr, ntm_msg msg)
{

	/**
	 *	Check whether the specified IP supports NTB transport. 
	 * 	 If not supported, return failed.
	 * 	 Else if supported:  
	 * 	 1. send `SYN` into target endpoint via traditional TCP/IP transport;
	 * 	 2. send `Dispatched` to nts process
	 * 	 3. (monitor) wait/receive `ACK` message from remote monitor
	 * 	 4. send `ntb channel setup` message into ntb-proxy to setup the NTB connection/queue
	 * 	 5. receive `setup ready` message<shm name of send/recv queue for the communication
	 * 		 between nts and ntb-proxy> from ntb-proxy.
	 * 	 6. send `ESTABLISHED` message with <shm name of send/recv queue> into nts process.
	 */

	nts_msg response_msg;
	nts_shm_conn_t nts_shm_conn;
	int retval;

	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = NTS_MSG_DISPATCHED;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if(!nts_shm_conn) {
		ERR("nts_shm_conn not found");
		goto FAIL;
	}

	if (nts_shm_conn->socket->state != CLOSED 
			|| nts_shm_conn->socket->state != BOUND) {
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_REQUIRE_CLOSED_OR_BOUND_FIRST;
		ERR("connect() requires socket() first");
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if (retval) {
			ERR("nts_shm_send failed for response to NT_ERR_REQUIRE_CLOSED_OR_BOUND_FIRST");
		}
		goto FAIL;
	}

	/**
	 * Check whether the target server ip is valid
	 */
	if (msg.addrlen <= 0) {
		// setting the errno as INVALID_IP
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_INVALID_IP;
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if (retval) {
			ERR("nts_shm_send failed for response to NT_ERR_INVALID_IP");
		}
		goto FAIL;
	}

	/**
	 * check whether the target server port is valid
	 */
	if (msg.port <= 0) {
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_INVALID_PORT;
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if(retval) {
			ERR("nts_shm_send failed for response to NT_ERR_INVALID_PORT");
		}

		goto FAIL;
	}

	/**
	 * if bound ip and port is all valid, then cache server ip/port
	 */
	nts_shm_conn->port = msg.port;
	nts_shm_conn->addrlen = msg.addrlen;
	memcpy(nts_shm_conn->ip, msg.address, msg.addrlen);
	
	
	/**
	 * Check whether the communication between local and remote ntm is setup (ntm_conn_context). 
	 * If the communication between local ntm and remote IP-specified ntm is not setup,
	 * 	setup the communication first.
	 */
	assert(ntm_mgr->ntm_conn_ctx);
	ntm_conn_t ntm_conn;
	if (Exists(ntm_mgr->ntm_conn_ctx->conn_map, msg.address)) {
		ntm_conn = (ntm_conn_t) Get(ntm_mgr->ntm_conn_ctx->conn_map, msg.address);
	}
	else {
		ntm_socket_t client_sock = ntm_sock_create();
		retval = ntm_connect_to_tcp_server(client_sock, msg.port, msg.address);
		if (retval) {
			ntm_close_socket(client_sock);

			response_msg.retval = -1;
			response_msg.nt_errno = NT_ERR_REMOTE_NTM_NOT_FOUND;
			retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
			if (retval) {
				ERR("nts_shm_send failed for response to NT_ERR_REMOTE_NTM_NOT_FOUND");
			}

			ntm_free(client_sock);
			return;
		}

		struct ntm_conn remote_ntm_conn;
		remote_ntm_conn.running_signal = true;
		remote_ntm_conn.client_sock = client_sock;
		remote_ntm_conn.sockfd = client_sock->socket_fd;
		remote_ntm_conn.port = msg.port;
		remote_ntm_conn.addrlen = strlen(msg.address);
		memcpy(remote_ntm_conn.ip, msg.address, remote_ntm_conn.addrlen);

		DEBUG("setup connection to remote monitor %s:%d", msg.address, msg.port);

		pthread_create(&remote_ntm_conn.recv_thr, NULL, 
						ntm_sock_recv_thread, &remote_ntm_conn);

		// cache the established ntm connection into hashmap
		// key: the ip address of remote monitor
		// value: struct ntm_conn
		Put(ntm_mgr->ntm_conn_ctx->conn_map, msg.address, &remote_ntm_conn);
		ntm_conn = &remote_ntm_conn;
		nts_shm_conn->ntm_conn = &remote_ntm_conn;
	}
	

	// TODO:
	// Check whether the specified IP supports NTB transport. 
	/* code here */


	// if NTB is supported and the client nt_socket state is not `BOUND`,
	// then automaticaly allocate an unused port for the client nt_socket
	nt_port_t port;
	if (nts_shm_conn->socket->state == CLOSED) {
		port = allocate_port(ntm_mgr->nt_port_ctx, 1);
		bzero(&nts_shm_conn->socket->saddr, sizeof(nts_shm_conn->socket->saddr));
		nts_shm_conn->socket->saddr.sin_family = AF_INET;
		nts_shm_conn->socket->saddr.sin_port = htons(port->port_id);
		nts_shm_conn->socket->saddr.sin_addr.s_addr = inet_addr(NTM_CONFIG.listen_ip);
		// cache the <port, nt_socket> mapping in `ntm_mgr->port_sock_map`
		Put(ntm_mgr->port_sock_map, &port->port_id, nts_shm_conn->socket);
	}


	// If NTB is supported: 
	// 1. dispatch `SYN` into target monitor via traditional TCP/IP transport;
	ntm_sock_msg syn_msg;
	syn_msg.type = NT_SYN;
	syn_msg.src_addr = nts_shm_conn->socket->saddr.sin_addr.s_addr;
	syn_msg.sport = nts_shm_conn->socket->saddr.sin_port;
	syn_msg.dst_addr = inet_addr(ntm_conn->ip);
	syn_msg.dport = htons(ntm_conn->port);
	retval = ntm_send_tcp_msg(ntm_conn->client_sock, (char*)&syn_msg, sizeof(syn_msg));
	if(retval <= 0) {
		ERR("ntm_send_tcp_msg send NT_SYN message failed");
		goto FAIL;
	}

	// 2. send `dispatched` message back to related nts app
	response_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if(retval) {
		ERR("nts_shm_send failed for response to NTS_MSG_DISPATCHED");
		goto FAIL;
	}

	DEBUG("handle_msg_nts_connect pass");


	FAIL:
	if(nts_shm_conn->socket->state == CLOSED) {
		if (port != NULL)
		{
			nts_shm_conn->socket->saddr.sin_port = htons(0);
			free_port(ntm_mgr->nt_port_ctx, port->port_id, 1);
		}
		
	}

	return;

}

inline void handle_msg_nts_bind(ntm_manager_t ntm_mgr, ntm_msg msg)
{
	nts_msg response_msg;
	nts_shm_conn_t nts_shm_conn;
	int retval;

	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = msg.msg_type;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		return;
	}

	if (nts_shm_conn->socket->state != CLOSED)
	{
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_REQUIRE_CLOSED_FIRST;
		ERR("bind() requires socket() first");
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if(retval) {
			ERR("nts_shm_send failed for response to NT_ERR_REQUIRE_CLOSED_FIRST");
		}

		return;
	}
	

	/**
	 * Check whether the bound ip is valid
	 */
	if (msg.addrlen <= 0) // or check invalid ip
	{
		// setting the errno as INVALID_IP
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_INVALID_IP;
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if (retval)
		{
			ERR("nts_shm_send failed for response to NT_ERR_INVALID_IP");
		}

		return;
	}

	/**
	 * Check whether the bound port is valid and idle
	 */
	// check if the port in valid range
	if (msg.port <= 0) // or other invalid range
	{
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_INVALID_PORT;
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if (retval)
		{
			ERR("nts_shm_send failed for response to NT_ERR_INVALID_PORT");
		}

		return;
	}

	// check if the port is idle
	// If used, nts_shm_send message to notify NT_ERR_INUSE_PORT
	if (is_occupied(ntm_mgr->nt_port_ctx, msg.port, NTM_CONFIG.max_port) == 1)
	{
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_INUSE_PORT;
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if (retval)
		{
			ERR("nts_shm_send failed for response to NT_ERR_INVALID_PORT");
		}

		return;
	}

	
	/**
	 * if bound ip and port is all valid,
	 *  then:
	 * 1. new struct nt_listener, nt_listener_wrapper
	 * 2. bind the <ip, port> with nt_listener
	 * 3. put the nt_listener_wrapper into `listener_map`(hash map) of `nt_listener_context`
	 */ 
	nts_shm_conn->port = msg.port;
	nts_shm_conn->addrlen = msg.addrlen;
	memcpy(nts_shm_conn->ip, msg.address, msg.addrlen);
	nts_shm_conn->socket->state = BOUND;

	/**
	 * send back success response to nts app
	 */
	response_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if (retval)
	{
		ERR("nts_shm_send failed for response to NTM_MSG_BIND");
		return;
	}
	
	DEBUG("handle_msg_nts_bind pass");
}

inline void handle_msg_nts_listen(ntm_manager_t ntm_mgr, ntm_msg msg)
{
	// assert(msg.msg_id > 0);
	// assert(msg.sockid > 0);

	nts_msg response_msg;
	nts_shm_conn_t nts_shm_conn;
	int retval;

	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = msg.msg_type;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		return;
	}
	
	// listen() requires `BOUND` state first
	// if current socket_state is not BOUND, then error
	if (nts_shm_conn->socket->state != BOUND)
	{
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_REQUIRE_BIND_FIRST;
		ERR("listen() require bind() first");
		nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		return;
	}

	if (msg.backlog > NTM_CONFIG.nt_max_conn_num)
	{
		msg.backlog = NTM_CONFIG.nt_max_conn_num;
	}
	

	/**
	 * If the socket state and backlog is valid, then:
	 * 1. new struct nt_listener, nt_listener_wrapper
	 * 2. bind the <ip, port> with nt_listener
	 * 3. put the nt_listener_wrapper into `listener_map`(hash map) of `nt_listener_context`
	 */
	struct nt_listener new_listener;
	struct nt_listener_wrapper new_listener_wrapper;

	// update the conresponding nt_socket: socket state, backlog
	new_listener.socket = nts_shm_conn->socket;
	new_listener.socket->socktype = NT_SOCK_LISTENER;
	new_listener.socket->listener = &new_listener;
	new_listener.socket->state = LISTENING;
	new_listener.socket->listener->backlog = msg.backlog;
	
	new_listener_wrapper.listener = &new_listener;
	new_listener_wrapper.port = nts_shm_conn->port;
	new_listener_wrapper.addrlen = nts_shm_conn->addrlen;
	memcpy(new_listener_wrapper.ip, nts_shm_conn->ip, nts_shm_conn->addrlen);
	new_listener_wrapper.nts_shm_conn = nts_shm_conn;
	new_listener_wrapper.accepted_conn_map = createHashMap(NULL, NULL);

	// setup backlog context
	sprintf(new_listener_wrapper.backlog_shmaddr, "backlog-%ld", new_listener.sockid);
	new_listener_wrapper.backlog_shmlen = strlen(new_listener_wrapper.backlog_shmaddr);
	new_listener_wrapper.backlog_ctx = backlog_ntm(&new_listener, 
							new_listener_wrapper.backlog_shmaddr, 
							new_listener_wrapper.backlog_shmlen, 
							new_listener.backlog);
	
	Put(ntm_mgr->nt_listener_ctx->listener_map, &new_listener_wrapper.port, &new_listener_wrapper);


	/**
	 * send back success response to nts app
	 */
	response_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if(retval){
		ERR("nts_shm_send failed for response to NTM_MSG_LISTEN");
		return;
	}

	DEBUG("handle_msg_nts_listen pass");
}

inline void handle_msg_nts_accept(ntm_manager_t ntm_mgr, ntm_msg msg)
{
	// assert(msg.msg_id > 0);
	// assert(msg.sockid > 0);

	nts_msg response_msg;
	nts_shm_conn_t nts_shm_conn;

	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = msg.msg_type;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		return;
	}

	// accept() requires `LISTENING` state first
	// if current socket_state is not LISTENING, then error
	if(nts_shm_conn->socket->state != LISTENING) {
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_REQUIRE_LISTEN_FIRST;
		ERR("accept() require listen() first");
		nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		return;
	}


	DEBUG("handle_msg_nts_accept pass");
}


inline void handle_msg_nts_accept_ack(ntm_manager_t ntm_mgr, ntm_msg msg) {
	nts_shm_conn_t nts_shm_conn;

	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		return;
	}

	/**
	 *	when recv `NTM_MSG_ACCEPT_ACK`, recv nts_shm name,	 
	 * 	create nts_shm_conn_t according to nts_shm name
	 */
	nts_shm_conn->running_signal = 1;
	if (msg.nts_shm_addrlen > 0){
		nts_shm_conn->shm_addrlen = msg.nts_shm_addrlen;
		memcpy(nts_shm_conn->nts_shm_name, msg.nts_shm_name, msg.nts_shm_addrlen);
	}

	int retval;
	nts_shm_conn->nts_shm_ctx = nts_shm();
	retval = nts_shm_connect(nts_shm_conn->nts_shm_ctx, msg.nts_shm_name, msg.nts_shm_addrlen);
	if (retval == -1) {
		ERR("new nts_shm_conn connect nts shm ringbuffer failed. ");
		nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
		nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
		return;
	}

	DEBUG("handle NTM_MSG_ACCEPT_ACK pass");

}


inline void handle_msg_nts_close(ntm_manager_t ntm_mgr, ntm_msg msg)
{

	/**
	 *  Receive the `SHUTDOWN` message from nts app.
	 * 	1. instruct ntb-proxy to destroy ntb connection/queue.
	 *  2. destroy the related resources in ntb monitor.
	 *  3. response the `SHUTDOWN Finished` message to nts app.
	 *  
	 */

	DEBUG("handle_msg_nts_close pass");
}

inline void handle_msg_nts_fin(ntm_manager_t ntm_mgr, ntm_msg msg) {
	
	/**
	 * 1. get nts_shm_conn via msg.sockid
	 * 2. check whether socket state is `ESTABLISHED` or not
	 * 3. check whether the target server ip and port is valid or not
	 * 4. send `NT_FIN_ACK` message to remote ntm via traditional TCP transport
	 * 5. set socket state to `WAIT_CLOSE`
	 * 6. destory client socket conn and nts_shm_conn
	 * 6. set socket state to `CLOSED`
	 */
	int retval;
	nts_shm_conn_t nts_shm_conn;
	nts_msg response_msg;

	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = NTS_MSG_SHUTDOWN;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if(!nts_shm_conn){
		ERR("nts_shm_conn not found");
		return;
	}

	if(nts_shm_conn->socket->state != ESTABLISHED){
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_REQUIRE_ESTABLISHED_FIRST;
		ERR("fin requires established first");
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if(retval){
			ERR("nts_shm_send failed for response to NT_ERR_REQUIRE_ESTABLISHED_FIRST");
		}
		return;
	}

	/**
	 * Check whether the target server ip is valid
 	 */
    if (msg.addrlen <= 0) {
        // setting the errno as INVALID_IP
        response_msg.retval = -1;
        response_msg.nt_errno = NT_ERR_INVALID_IP;
        retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
        if (retval) {
            ERR("nts_shm_send failed for response to NT_ERR_INVALID_IP");
        }
        return;
    }
	/**
     * check whether the target server port is valid
     */
	if (msg.port <= 0) {
        response_msg.retval = -1;
        response_msg.nt_errno = NT_ERR_INVALID_PORT;
        retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
        if(retval) {
            ERR("nts_shm_send failed for response to NT_ERR_INVALID_PORT");
        }
	    return;
    }

	assert(ntm_mgr->ntm_conn_ctx);
	ntm_conn_t ntm_conn;
	if(Exists(ntm_mgr->ntm_conn_ctx->conn_map, msg.address)){
		ntm_conn = (ntm_conn_t) Get(ntm_mgr->ntm_conn_ctx->conn_map, msg.address);
	}else {
		ERR("the communication between local ntm and remote ntm is not setup");
		return;
	}
	
	ntm_sock_msg sock_msg;
	sock_msg.src_addr = nts_shm_conn->socket->saddr.sin_addr.s_addr;
	sock_msg.sport = nts_shm_conn->socket->saddr.sin_port;
	sock_msg.dst_addr = inet_addr(ntm_conn->ip);
	sock_msg.dport = htons(ntm_conn->port);
	sock_msg.type = NT_FIN_ACK;
	retval = ntm_send_tcp_msg(ntm_conn->client_sock, (char*)&sock_msg, sizeof(sock_msg));
	if(retval <= 0){
		ERR("ntm_send_tcp_msg send NT_FIN_ACK message failed");
		return;
	}
	// set state to WAIT_CLOSE
	nt_socket_t sock = get_socket(ntm_mgr->nt_sock_ctx, msg.sockid, NTM_CONFIG.max_concurrency);
	if(!sock){
		ERR("Non-exist the socket gotten by msg.sockid");
		return;
	}
	sock->state = WAIT_CLOSE;
	destroy_client_nt_socket_conn_with_nts_shm_conn(nts_shm_conn);
	sock->state = CLOSED;

	// set state to CLOSED
	DEBUG("handle_msg_nts_fin pass");
}

inline void handle_msg_err(ntm_manager_t ntm_mgr, ntm_msg msg)
{

	DEBUG("handle_msg_err pass");
}

void *nts_shm_recv_thread(void *args)
{
	assert(ntm_mgr);
	assert(ntm_mgr->nts_ctx->shm_recv_ctx);

	DEBUG("nts_recv_thread ready...");
	ntm_nts_context_t nts_ctx;
	nts_ctx = ntm_mgr->nts_ctx;
	
	ntm_msg recv_msg;
	nts_ctx->shm_recv_signal = 1; // if 1, running; else stop

	while (nts_ctx->shm_recv_signal)
	{
		int retval;
		retval = ntm_shm_recv(nts_ctx->shm_recv_ctx, &recv_msg);
		if (retval == 0)
		{
			DEBUG("receive a message");
			nts_shm_handle_msg(ntm_mgr, recv_msg);
		}
		else
		{
			break;
		}
	}

	DEBUG("nts_recv_thread end!");

	return NULL;
}

void nts_shm_handle_msg(ntm_manager_t ntm_mgr, ntm_msg msg)
{
	assert(ntm_mgr);

	DEBUG("nts shm handler function");

	if (msg.msg_type & NTM_MSG_INIT)
	{
		DEBUG("handle NTM_MSG_INIT");
	}
	else if (msg.msg_type & NTM_MSG_NEW_SOCK)
	{
		DEBUG("handle NTM_MSG_NEW_SOCK");

		handle_msg_nts_new_socket(ntm_mgr, msg);
	}
	else if (msg.msg_type & NTM_MSG_BIND)
	{
		DEBUG("handle NTM_MSG_BIND");

		handle_msg_nts_bind(ntm_mgr, msg);
	}
	else if (msg.msg_type & NTM_MSG_LISTEN)
	{
		DEBUG("handle NTM_MSG_LISTEN");

		handle_msg_nts_listen(ntm_mgr, msg);
	}
	else if (msg.msg_type & NTM_MSG_ACCEPT)
	{
		DEBUG("handle NTM_MSG_ACCEPT");

		// handle_msg_nts_accept(ntm_mgr, msg);
	}
	else if (msg.msg_type & NTM_MSG_ACCEPT_ACK)
	{
		DEBUG("handle NTM_MSG_ACCEPT");

		handle_msg_nts_accept_ack(ntm_mgr, msg);
	}
	else if (msg.msg_type & NTM_MSG_CONNECT)
	{
		DEBUG("handle NTM_MSG_CONNECT");

		handle_msg_nts_connect(ntm_mgr, msg);
	}
	else if (msg.msg_type & NTM_MSG_ESTABLISH)
	{
		DEBUG("handle NTM_MSG_ESTABLISH");
	}
	else if (msg.msg_type & NTM_MSG_CLOSE)
	{
		DEBUG("handle NTM_MSG_CLOSE");

		handle_msg_nts_close(ntm_mgr, msg);
	}
	else if (msg.msg_type & NTM_MSG_FIN)
	{
		DEBUG("handle NTM_MSG_FIN");

		handle_msg_nts_fin(ntm_mgr, msg);
	}
	else
	{
		DEBUG("Error NTM Message Type");

		handle_msg_err(ntm_mgr, msg);
	}
}

int print_monitor()
{
	printf("Hello Ntb Monitor.\n");

	//	test_ntm_ring();
	//	test_ntm_shm();
	//	test_nts_shm();
	printf("Bye, Ntb Monitor.\n");

	return 0;
}

inline bool try_close_ntm_listener(ntm_conn_ctx_t ntm_conn_ctx) {
	assert(ntm_conn_ctx);

	int retval;
	ntm_socket_t client_sock = ntm_sock_create();
	ntm_conn_ctx->running_signal = false;
	retval = ntm_connect_to_tcp_server(client_sock, 
										ntm_conn_ctx->listen_port, ntm_conn_ctx->listen_ip);
	if(retval) {
		ntm_close_socket(client_sock);
		return false;
	}

	return true;

}

inline void destroy_client_nt_socket_conn(nt_socket_t client_socket, 
				nts_shm_conn_t nts_shm_conn, int bound_port) {
	assert(ntm_mgr);
	assert(client_socket);
	assert(client_socket->sockid > 0);
	assert(nts_shm_conn);
	assert(bound_port > 0);

	// Remove `client_socket` from `ntm_mgr->port_sock_map`
	if (Exists(ntm_mgr->port_sock_map, &bound_port))
		Remove(ntm_mgr->port_sock_map, &bound_port);

	// free nt_port 
	free_port(ntm_mgr->nt_port_ctx, bound_port, 1);

	// close and destroy nts_shm_context_t in `nts_shm_conn`
	// free nts_shm_conn
	if(nts_shm_conn) {
		// if accepted client socket (created by `accept()`),
		// then Remove coresponding `nt_socket_t` in `accepted_conn_map` of `nt_listener_wrapper`
		if (client_socket->socktype == NT_SOCK_PIPE) {
			if (nts_shm_conn->listener && 
					Exists(nts_shm_conn->listener->accepted_conn_map, &client_socket->sockid)) {
				Remove(nts_shm_conn->listener->accepted_conn_map, &client_socket->sockid);
			}
		}

		if (nts_shm_conn->nts_shm_ctx) {
			nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
			nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
		}

		if (Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid)) {
			Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid);
		}
		free(nts_shm_conn);

	}
	
	if (client_socket) {
		free_socket(ntm_mgr->nt_sock_ctx, client_socket->sockid, 1);
	}


}

inline void destroy_client_nt_socket_conn_with_sock(nt_socket_t client_socket) {
	assert(ntm_mgr);
	assert(client_socket);
	assert(client_socket->sockid > 0);

	nts_shm_conn_t nts_shm_conn;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid);
	
	int bound_port;
	bound_port = parse_sockaddr_port(&client_socket->saddr);

	if (nts_shm_conn && bound_port)
	{
		destroy_client_nt_socket_conn(client_socket, nts_shm_conn, bound_port);
		return;
	}
	else if (bound_port > 0)
	{
		// Remove `client_socket` from `ntm_mgr->port_sock_map`
		if (Exists(ntm_mgr->port_sock_map, &bound_port))
			Remove(ntm_mgr->port_sock_map, &bound_port);

		// free nt_port 
		free_port(ntm_mgr->nt_port_ctx, bound_port, 1);
	} else { // nts_shm_conn exists
		// close and destroy nts_shm_context_t in `nts_shm_conn`
	// free nts_shm_conn
	if(nts_shm_conn) {
		// if accepted client socket (created by `accept()`),
		// then Remove coresponding `nt_socket_t` in `accepted_conn_map` of `nt_listener_wrapper`
		if (client_socket->socktype == NT_SOCK_PIPE) {
			if (nts_shm_conn->listener && 
					Exists(nts_shm_conn->listener->accepted_conn_map, &client_socket->sockid)) {
				Remove(nts_shm_conn->listener->accepted_conn_map, &client_socket->sockid);
			}
		}

		if (nts_shm_conn->nts_shm_ctx) {
			nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
			nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
		}

		if (Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid)) {
			Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid);
		}
		free(nts_shm_conn);

	}
	}

	if (client_socket) {
		free_socket(ntm_mgr->nt_sock_ctx, client_socket->sockid, 1);
	}

}

inline void destroy_client_nt_socket_conn_with_nts_shm_conn(nts_shm_conn_t nts_shm_conn) {
	assert(ntm_mgr);
	assert(nts_shm_conn);

	nt_socket_t client_socket;
	int bound_port;

	if (nts_shm_conn->socket)
	{
		destroy_client_nt_socket_conn_with_sock(nts_shm_conn->socket);
		return;
	} else {
		int bound_port;
		bound_port = parse_sockaddr_port(&client_socket->saddr);
		if (bound_port > 0) {
			// Remove `client_socket` from `ntm_mgr->port_sock_map`
			if (Exists(ntm_mgr->port_sock_map, &bound_port))
				Remove(ntm_mgr->port_sock_map, &bound_port);

			// free nt_port 
			free_port(ntm_mgr->nt_port_ctx, bound_port, 1);
		}
	}
	


}

