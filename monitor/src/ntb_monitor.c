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
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

#include "ntb_monitor.h"
#include "config.h"
#include "ntm_shmring.h"
#include "ntm_shm.h"
#include "nt_errno.h"

#include "epoll_event_queue.h"
#include "epoll_msg.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define MSG "Hello NT-Monitor! I am libnts app. Nice to meet you!"

static inline bool try_close_ntm_listener(ntm_conn_ctx_t ntm_conn_ctx);



/**
 * destroy nt_socket-level nt_socket-related resources, i.e., nts_shm_conn, nts_shm_ctx, nt_socket_id, nt_port
 * 		nts_shm_context, accepted_conn_map, port_sock_map, nts_shm_conn_map(ntm_nts_context)
 */
static inline void destroy_client_nt_socket_conn(nt_socket_t client_socket, nts_shm_conn_t nts_shm_conn, int bound_port);
static inline void destroy_client_nt_socket_conn_with_sock(nt_socket_t client_socket);
static inline void destroy_client_nt_socket_conn_with_nts_shm_conn(nts_shm_conn_t nts_shm_conn);


ntm_manager_t get_ntm_manager()
{
	assert(ntm_mgr);
	return ntm_mgr;
}

int ntm_init(const char *config_file)
{

	/**
	 * register signal callback functions
	 */
	signal(SIGINT, ntm_destroy);
	// signal(SIGKILL, ntm_destroy);
	// signal(SIGSEGV, ntm_destroy);
	// signal(SIGTERM, ntm_destroy);

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
	ret = init_port_context(ntm_mgr->nt_port_ctx, NTM_CONFIG.max_port);
	if (ret == -1) {
		ERR("init_port_context failed");
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
		ERR("Failed to allocate ntm_conn_context.");
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
		ERR("Failed to allocate nt_listener_context.");
		goto FAIL;
	}
	nt_listener_ctx = ntm_mgr->nt_listener_ctx;
	// hash map for holding all nt_listener indexed by port
	nt_listener_ctx->listener_map = createHashMap(NULL, NULL);


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
	 *	for epoll manager
	 */
	ntm_mgr->epoll_mgr = (epoll_manager_t)calloc(1, sizeof(struct epoll_manager));
	if (!ntm_mgr->epoll_mgr)
	{
		perror(err_msg);
		ERR("Failed to allocate epoll_manager.");
		goto FAIL;
	}
	// hashmap for the mapping between epoll-fd and epoll_context
	ntm_mgr->epoll_mgr->epoll_ctx_map = createHashMap(NULL, NULL);

	// connect/init the 'ntp_ep_send_queue' and 'ntp_ep_recv_queue' epoll ring queue.
	epoll_sem_shm_ctx_t ntp_ep_send_ctx;
	ntp_ep_send_ctx = epoll_sem_shm();
	if (!ntp_ep_send_ctx) {
		perror(err_msg);
		ERR("Failed to allocate memory for 'epoll_sem_shm_ctx_t ntp_ep_send_ctx'.");
		goto FAIL;
	}
	ret = epoll_sem_shm_connect(ntp_ep_send_ctx, NTP_EP_SEND_QUEUE, strlen(NTP_EP_SEND_QUEUE));
	if (ret != 0) {
		ERR("Failed to epoll_sem_shm_connect for 'epoll_sem_shm_ctx_t ntp_ep_send_ctx'.");
		goto FAIL;
	}
	ntm_mgr->epoll_mgr->ntp_ep_send_ctx = ntp_ep_send_ctx;

	epoll_sem_shm_ctx_t ntp_ep_recv_ctx;
	ntp_ep_recv_ctx = epoll_sem_shm();
	if (!ntp_ep_recv_ctx) {
		perror(err_msg);
		ERR("Failed to allocate memory for 'epoll_sem_shm_ctx_t ntp_ep_recv_ctx'.");
		goto FAIL;
	}
	ret = epoll_sem_shm_connect(ntp_ep_recv_ctx, NTP_EP_RECV_QUEUE, strlen(NTP_EP_RECV_QUEUE));
	if (ret != 0) {
		ERR("Failed to epoll_sem_shm_connect for 'epoll_sem_shm_ctx_t ntp_ep_recv_ctx'.");
		goto FAIL;
	}
	ntm_mgr->epoll_mgr->ntp_ep_recv_ctx = ntp_ep_recv_ctx;

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

	DEBUG("ntm_init success");

	nts_shm_recv_thread(NULL);

	DEBUG("ntm_init fully pass");

	return 0;

	FAIL:

	return -1;

}

void ntm_destroy()
{
	assert(ntm_mgr);
	DEBUG("nt_destroy ready...");
	bool ret;
	void *status;

	if(ntm_mgr->nts_ctx)
		ntm_mgr->nts_ctx->shm_recv_signal = 0;


	/**
	 * Destroy the ntp context resources
	 */
	DEBUG("Destroy ntp context resources");

	
	/**
	 * Disconnect and exit ntm_listener accept thread `ntm_conn_ctx->ntm_sock_listen_thr` 
	 */
	ret = try_close_ntm_listener(ntm_mgr->ntm_conn_ctx);
	if(!ret) {
		ERR("try close ntm listener failed.");
	}
	
	pthread_join(ntm_mgr->ntm_conn_ctx->ntm_sock_listen_thr, &status);

	
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
		ntm_conn = (ntm_conn_t) iter->entry->value;

		/**
		 * Disconnect and exit `client_conn->recv_thr` [ntm_sock_recv_thread()]
		 * 	in 'ntm_mgr->ntm_conn_ctx->conn_map'
		 */
		ntm_conn->running_signal = false;
		shutdown(ntm_conn->client_sock->socket_fd, SHUT_RDWR);
		pthread_join(ntm_conn->recv_thr, &status);

		ntm_close_socket(ntm_conn->client_sock);
		Remove(ntm_mgr->ntm_conn_ctx->conn_map, ntm_conn->ip);
		free(ntm_conn);
	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->ntm_conn_ctx->conn_map);
	ntm_mgr->ntm_conn_ctx->conn_map = NULL;
	DEBUG("free hashmap for ntm_conn context success");

	if (ntm_mgr->ntm_conn_ctx->server_sock)
	{
		DEBUG("close server_sock");
		ntm_close_socket(ntm_mgr->ntm_conn_ctx->server_sock);
	}
	// if(ntm_mgr->ntm_conn_ctx->listen_ip) {
	// 	DEBUG("free listen_ip");
	// 	free(ntm_mgr->ntm_conn_ctx->listen_ip);
	// }
	DEBUG("Destroy ntm listen socket resources success");

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
		nt_listener_wrapper = (nt_listener_wrapper_t)iter->entry->value;

		nt_conn_iter = createHashMapIterator(nt_listener_wrapper->accepted_conn_map);

		while (hasNextHashMapIterator(nt_conn_iter))
		{
			nt_conn_iter = nextHashMapIterator(iter);
			tmp_socket = (nt_socket_t) nt_conn_iter->entry->value;

			Remove(nt_listener_wrapper->accepted_conn_map, &tmp_socket->sockid);
		}
		freeHashMapIterator(&nt_conn_iter);
		Clear(nt_listener_wrapper->accepted_conn_map);
		nt_listener_wrapper->accepted_conn_map = NULL;
		DEBUG("free hash map for client socket connection accepted by the specified nt_listener socket success");

		// free or unbound nt_listener socket id
		Remove(ntm_mgr->nt_listener_ctx->listener_map, &nt_listener_wrapper->port);
		free_socket(ntm_mgr->nt_sock_ctx, nt_listener_wrapper->listener->sockid, 1);
		free(nt_listener_wrapper);
	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->nt_listener_ctx->listener_map);
	ntm_mgr->nt_listener_ctx->listener_map = NULL;
	DEBUG("Destroy the nt_listener related resources success");

	/**
	 * Destroy the ntm-nts context resources
	 */
	nts_shm_conn_t nts_shm_conn;
	iter = createHashMapIterator(ntm_mgr->nts_ctx->nts_shm_conn_map);
	while (hasNextHashMapIterator(iter))
	{
		iter = nextHashMapIterator(iter);
		nts_shm_conn = (nts_shm_conn_t) iter->entry->value;

		// free or unbound socket id
		DEBUG("free_socket");
		free_socket(ntm_mgr->nt_sock_ctx, nts_shm_conn->sockid, 1);

		// free nts_shm_context
		if (nts_shm_conn->nts_shm_ctx && nts_shm_conn->nts_shm_ctx->shm_stat == NTS_SHM_READY)
		{
			DEBUG("nts_shm_conn->nts_shm_ctx close start");
			nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
			DEBUG("nts_shm_destroy start");
			nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
		}

		Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, &nts_shm_conn->sockid);
		DEBUG("free nts_shm_conn");
		free(nts_shm_conn);
	}
	freeHashMapIterator(&iter);
	Clear(ntm_mgr->nts_ctx->nts_shm_conn_map);
	ntm_mgr->nts_ctx->nts_shm_conn_map = NULL;
	DEBUG("free hash map for nts_shm_conn success");


	// Destroy the port_sock_map hashmap
	Clear(ntm_mgr->port_sock_map);
	ntm_mgr->port_sock_map = NULL;

	/**
	 * Destroy the ntp-related (ntp <==> ntm) resources/context, including:
	 * 1. destroy ntp ==> ntm shm recv queue
	 * 2. destroy ntm ==> ntp shm send queue
	 */
	ntm_ntp_context_t ntp_ctx;
	ntp_ctx = ntm_mgr->ntp_ctx;
	if (ntp_ctx->shm_send_ctx && 
				ntp_ctx->shm_send_ctx->shm_stat == SHM_STAT_READY) {
		DEBUG("ntp_ctx->shm_send_ctx close");
		ntm_ntp_shm_ntm_close(ntp_ctx->shm_send_ctx);
		DEBUG("ntm_ntp_shm_destroy");
		ntm_ntp_shm_destroy(ntp_ctx->shm_send_ctx);
	}
	if (ntp_ctx->shm_recv_ctx &&
				ntp_ctx->shm_recv_ctx->shm_stat == SHM_STAT_READY) {
		DEBUG("ntp_ctx->shm_recv_ctx close");
		ntp_ntm_shm_ntm_close(ntp_ctx->shm_recv_ctx);
		ntp_ntm_shm_destroy(ntp_ctx->shm_recv_ctx);
	}
	DEBUG("Destroy the ntm-ntp context resources success");


	ntm_shm_context_t shm_recv_ctx;
	shm_recv_ctx = ntm_mgr->nts_ctx->shm_recv_ctx;
	if (shm_recv_ctx && shm_recv_ctx->shm_stat == NTM_SHM_READY)
	{
		ntm_shm_nts_close(shm_recv_ctx);
		ntm_shm_destroy(shm_recv_ctx);
		shm_recv_ctx = NULL;
	}
	DEBUG("Destroy the ntm-nts context resources success");


	//	nts_shm_context_t shm_send_ctx;
	//	shm_send_ctx = ntm_mgr->nts_ctx->shm_send_ctx;
	//	if (shm_send_ctx && shm_send_ctx->shm_stat == NTS_SHM_READY) {
	//		nts_shm_ntm_close(shm_send_ctx);
	//		nts_shm_destroy(shm_send_ctx);
	//	}

	/**
	 * Destroy the context structure
	 */
	if(ntm_mgr->ntm_conn_ctx) {
		free(ntm_mgr->ntm_conn_ctx);
		ntm_mgr->ntm_conn_ctx = NULL;
	}
	if(ntm_mgr->nt_listener_ctx) {
		free(ntm_mgr->nt_listener_ctx);
		ntm_mgr->nt_listener_ctx = NULL;
	}
	if(ntm_mgr->ntp_ctx){
		free(ntm_mgr->ntp_ctx);
		ntm_mgr->ntp_ctx = NULL;
	}
	if(ntm_mgr->nts_ctx) {
		free(ntm_mgr->nts_ctx);
		ntm_mgr->nts_ctx = NULL;
	}
	
	free(ntm_mgr);
	ntm_mgr = NULL;

	/**
     * destroy the memory which is allocated to NTM_CONFIG
     */
    free_conf();

	DEBUG("ntm_destroy success");

	exit(0);
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
static inline int handle_nt_client_syn_ack_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);
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
	outgoing_msg.dport = msg.sport;
	outgoing_msg.dst_addr = msg.src_addr;

	// check whether the SYN message is valid or not
	int sport, dport;
	sport = ntohs(msg.sport);
	dport = ntohs(msg.dport);
	DEBUG("handle_nt_syn_msg with sport=%d, dport=%d", sport, dport);
	if (sport <0 || dport <0)
	{
		outgoing_msg.type = NT_INVALID_PORT;
		ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		return;
	}
	
	// check whether the specified dport has conresponding nt_listener
	if(!Exists(ntm_mgr->nt_listener_ctx->listener_map, &dport)){
		ERR("nt listener not found with nt_port=%d", dport);
		outgoing_msg.type = NT_LISTENER_NOT_FOUND;
		ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		return;
	}

	nt_listener_wrapper_t listener_wrapper;
	listener_wrapper = (nt_listener_wrapper_t)Get(ntm_mgr->nt_listener_ctx->listener_map, &dport);
	// printf("listener_wrapper %p \n", listener_wrapper);
	// printf("listener_wrapper->listener %p\n", listener_wrapper->listener);
	DEBUG("gain nt_listener according to dport[%d] with nt_socket state %d", dport, listener_wrapper->listener->socket->state);
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
	// bzero(&client_socket->saddr, sizeof(struct sockaddr_in));
	client_socket->saddr.sin_family = AF_INET;
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
	DEBUG("instruct ntb-proxy to setup the ntb connection/queue");
	ntm_ntp_msg ntp_outgoing_msg;	// for send msg into ntp
	int retval;
	
	ntp_outgoing_msg.dst_ip = msg.src_addr;
	ntp_outgoing_msg.dst_port = msg.sport;
	ntp_outgoing_msg.src_ip = msg.dst_addr;
	ntp_outgoing_msg.src_port = msg.dport;
	ntp_outgoing_msg.msg_type = 1;
	ntp_outgoing_msg.msg_len = 0;
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
	
	INFO("ntp_incoming_msg.msg_type=%d, msg info=%s", ntp_incoming_msg.msg_type, ntp_incoming_msg.msg);
	// if ntb connection setup failed, goto FAIL.
	if (ntp_incoming_msg.msg_type != 1) {
		ERR("ntb connection setup failed with %d", ntp_incoming_msg.msg_type);
		goto FAIL;
	}


	DEBUG("recv the response message of `ntb connection setup successfully` from ntp.");

	/** 
	 * TODO: if receive `SETUP Success` message from ntb-proxy (ntp_incoming_msg.msg_type == 1),
	 * 	init/create coresponding `nts_shm_conn_t` for the accepted nt_socket,
	 * 	push new nts shm conn into hashmap,
	 * 	update client socket state as `WAIT_ESTABLISHED`
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
	new_nts_shm_conn->ntm_conn = ntm_conn;

	/**
	 * set `peer_sin_port` and `peer_sin_addr` after setup ntb connection in ntb proxy
	 */
	new_nts_shm_conn->peer_sin_addr = msg.src_addr;
	new_nts_shm_conn->peer_sin_port = msg.sport;

	// push new nts shm conn into hashmap
	DEBUG("push new nts shm conn into hash map with sockid=%d", client_socket->sockid);
	Put(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_socket->sockid, new_nts_shm_conn);

	// update client socket state as `WAIT_ESTABLISHED`
	client_socket->state = WAIT_ESTABLISHED;

	// push the new `client_socket` into coresponding `listener_wrapper->accepted_conn_map`
	Put(listener_wrapper->accepted_conn_map, &client_socket->sockid, client_socket);

	DEBUG("backlog_push one client nt_socket with sockid=%d", client_socket->sockid);
	retval = backlog_push(listener_wrapper->backlog_ctx, client_socket);
	if(retval == -1) {
		ERR("backlog_push one client nt_socket failed");
		goto FAIL;
	}

	// after backlog_push, check whether epoll or not
	int rc;
	nts_shm_conn_t listen_conn;
	listen_conn = listener_wrapper->nts_shm_conn;
	if(listen_conn->socket->epoll && (listen_conn->socket->epoll & NTS_EPOLLIN)) {
		nts_epoll_event_int event;
		event.sockid = listen_conn->socket->sockid;
		event.ev.data = listen_conn->socket->sockid;
		event.ev.events = NTS_EPOLLIN;
		rc = ep_event_queue_push(listen_conn->epoll_ctx->ep_io_queue_ctx, &event);
		if (rc) {
			ERR("ep_event_queue_push failed for listener socket NTS_EPOLLIN event");
		}
	}

	// set the allocated nt_port sin_port of accepted nt_socket as source port in `outgoing_msg.sport`
	// outgoing_msg.sport = client_socket->saddr.sin_port;
	outgoing_msg.sport = msg.dport;
	outgoing_msg.dport = msg.sport;
	sport = ntohs(outgoing_msg.sport);
	dport = ntohs(outgoing_msg.dport);
	DEBUG("SYN_ACK outgoing msg with sport=%d, dport=%d", sport, dport);

	// send `SYN_ACK` with local client nt_socket id back to source monitor.
	outgoing_msg.sockid = client_socket->sockid;
	outgoing_msg.type = NT_SYN_ACK;
	ntm_send_tcp_msg(ntm_conn->client_sock, 
				(char*)&outgoing_msg, sizeof(outgoing_msg));

	DEBUG("handle NT_SYN message success");
	return;

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

	if (new_nts_shm_conn) {
		free(new_nts_shm_conn);
	}

	return;
}

inline void handle_nt_syn_ack_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {
	assert(msg.sockid > 0);

	int retval;
	
	int sport, dport;
	sport = ntohs(msg.sport);
	dport = ntohs(msg.dport);

	// check whether the SYN_ACK message is valid or not
	if (sport < 0 || dport < 0) {
		ERR("Invalid port value.");
		return;
	}

	// locate the coressponding client nt_client via `dport`
	DEBUG("nt_syn_ack_msg: dport=%d, sport=%d", dport, sport);
	HashMapIterator iter = createHashMapIterator(ntm_mgr->port_sock_map);
	while(hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		// nt_socket_t tmp_sock = (nt_socket_t)iter->entry->value;
		DEBUG("{ key = %d }", *(int *) iter->entry->key);
	}
	freeHashMapIterator(&iter);

	nt_socket_t client_sock;
	client_sock = (nt_socket_t) Get(ntm_mgr->port_sock_map, &dport);
	if (!client_sock) {
		ERR("Invalid or Non-existing destination port %d.", dport);
		return;
	}

	/**
	 * receive `NT_SYN_ACK`, then tell ntp to setup ntb connection,
	 * i. setup ntp msg and send to ntp shm
	 * ii. wait for the response msg from ntp
	 *  update the nt_socket state as `ESTABLISHED`.
	 * iii. return `CLIENT_SYN_ACK` msg to remote monitor
	 */
	DEBUG("instruct ntp to setup ntb connection");
	
	ntm_ntp_msg ntp_outgoing_msg;	// for send msg into ntp
	ntp_outgoing_msg.dst_ip = msg.src_addr;
	ntp_outgoing_msg.dst_port = msg.sport;
	ntp_outgoing_msg.src_ip = msg.dst_addr;
	ntp_outgoing_msg.src_port = msg.dport;
	ntp_outgoing_msg.msg_type = 1;
	ntp_outgoing_msg.msg_len = 0;
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

	// return `CLIENT_SYN_ACK` msg to remote monitor
	ntm_sock_msg resp_sock_msg;
	resp_sock_msg.sockid = msg.sockid;
	resp_sock_msg.src_addr = msg.dst_addr;
	resp_sock_msg.sport = msg.dport;
	resp_sock_msg.dport = msg.sport;
	resp_sock_msg.dst_addr = msg.src_addr;
	resp_sock_msg.type = NT_CLIENT_SYN_ACK;
	ntm_send_tcp_msg(ntm_conn->client_sock, 
				(char*)&resp_sock_msg, sizeof(resp_sock_msg));
	DEBUG("return `CLIENT_SYN_ACK` msg to remote monitor success");

	if (!Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid))
	{
		ERR("The nts_shm_conn of client socket[sockid=%d, port=%d] is not found", (int)client_sock->sockid, dport);
		return;
	}
	nts_shm_conn_t nts_shm_conn;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid);

	/**
	 * set `peer_sin_port` and `peer_sin_addr` after setup ntb connection in ntb proxy
	 */
	nts_shm_conn->peer_sin_port = msg.sport;
	nts_shm_conn->peer_sin_addr = msg.src_addr;


	nts_msg response_msg;
	response_msg.msg_type = NTS_MSG_ESTABLISH;
	response_msg.sockid = client_sock->sockid;
	
	response_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if(retval) {
		ERR("nts_shm_send failed for response to NTS_MSG_ESTABLISH");
		return;
	}

	DEBUG("handle NT_SYN_ACK message success");
	return;

	FAIL: 

	return;
}

inline int handle_nt_client_syn_ack_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {
   /**
 	*  Specially for Server nt_socket located at ntb monitor
 	*	Receive `CLIENT SYN ACK` message from remote monitor.
	*	1. check whether the CLIENT SYN ACK is valid
	*	2. if valid:
	*	 i. update corresponding client nt_socket state as `ESTABLISHED`
	*	 ii. forward `CLIENT_SYN_ACK` nts_msg into corresponding client nt_socket via nts_shm
	* 
	*/	
	assert(msg.sockid > 0);
	int retval = 1;
	
	ntm_sock_msg outgoing_msg;
	outgoing_msg.src_addr = msg.dst_addr;
	outgoing_msg.dport = msg.sport;
	outgoing_msg.dst_addr = msg.src_addr;

	// check whether the SYN message is valid or not
	int sport, dport;
	sport = ntohs(msg.sport);
	dport = ntohs(msg.dport);
	DEBUG("handle_nt_syn_msg with sport=%d, dport=%d", sport, dport);
	if (sport <0 || dport <0)
	{
		ERR("NT INVALID PORT in nt_client_syn_ack_msg");
		outgoing_msg.type = NT_INVALID_PORT;
		retval = ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		goto FAIL;
	}

	// check whether the specified dport has conresponding nt_listener
	if(!Exists(ntm_mgr->nt_listener_ctx->listener_map, &dport)){
		ERR("nt listener not found with nt_port=%d", dport);
		outgoing_msg.type = NT_LISTENER_NOT_FOUND;
		retval = ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		goto FAIL;
	}

	nt_listener_wrapper_t listener_wrapper;
	listener_wrapper = (nt_listener_wrapper_t)Get(ntm_mgr->nt_listener_ctx->listener_map, &dport);
	DEBUG("gain nt_listener according to dport[%d] with nt_socket state %d", dport, listener_wrapper->listener->socket->state);
	if (listener_wrapper->listener->socket->state != LISTENING)
	{
		outgoing_msg.type = NT_LISTENER_NOT_READY;
		retval = ntm_send_tcp_msg(ntm_conn->client_sock, 
						(char*)&outgoing_msg, sizeof(outgoing_msg));
		goto FAIL;
	}


	// 2. if valid:
	// 	 i. update corresponding client nt_socket state as `ESTABLISHED`
	// 	 ii. forward `CLIENT_SYN_ACK` nts_msg into corresponding server listening nt_socket via nts_shm
	nt_socket_t client_socket;
	client_socket = (nt_socket_t) Get(listener_wrapper->accepted_conn_map, &msg.sockid);
	client_socket->state = ESTABLISHED;


	nts_msg resp_msg;
	resp_msg.sockid = msg.sockid;
	resp_msg.msg_type = NTS_MSG_CLIENT_SYN_ACK;
	resp_msg.retval = 0;
	retval = nts_shm_send(listener_wrapper->nts_shm_conn->nts_shm_ctx, &resp_msg);
	if(retval) {
		ERR("nts_shm_send failed for NTS_MSG_CLIENT_SYN_ACK msg");
		goto FAIL;
	}


	DEBUG("handle NT_CLIENT_SYN_ACK message success");

	return 0;

	FAIL: 
	// if (retval <= 0) {
	// 	return -1;
	// }

	return -1;

}


inline void handle_nt_invalid_port_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_INVALID_PORT message success");

}

 
inline void handle_nt_listener_not_found_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_LISTENER_NOT_FOUND message success");

}

inline void handle_nt_listener_not_ready_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_LISTENER_NOT_READY message success");

}

inline void handle_backlog_is_full_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_BACKLOG_IS_FULL success");

}


inline void handle_nt_fin_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle NT_FIN success");
	
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
	if (sport < 0 || dport < 0) {
		ERR("Invalid port value.");
		goto FAIL;
	}

	// locate the coressponding client nt_client via `dport`
	nt_socket_t client_sock;
	client_sock = (nt_socket_t) Get(ntm_mgr->port_sock_map, &dport);
	if (!client_sock) {
		ERR("Invalid or Non-existing destination port.");
		goto FAIL;
	}

	// update the nt_socket state as `WAIT_CLOSE`
	client_sock->state = WAIT_CLOSE;

	// judge whether the coresponding `nts_shm_conn` exists or not
	// if exists, get it
	if (!Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &client_sock->sockid))
	{
		ERR("The nts_shm_conn of client socket[sockid=%d, port=%d] is not found", (int)client_sock->sockid, dport);
		goto FAIL;
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
		goto FAIL;
	}

	client_sock->state = CLOSED;	


	/**
	 * 4. destroy nt_socket-related resources, i.e., nts_shm_conn, nts_shm_ctx, nt_socket_id, nt_port
	 * 		nts_shm_context, accepted_conn_map, port_sock_map, nts_shm_conn_map(ntm_nts_context)
	 */
	DEBUG("dport=%d, sport=%d.", dport, sport);
	destroy_client_nt_socket_conn(client_sock, nts_shm_conn, dport);
	
	// Remove `client_socket` from `ntm_mgr->port_sock_map`
	// if (Exists(ntm_mgr->port_sock_map, &dport))
	// 	Remove(ntm_mgr->port_sock_map, &dport);

	// free nt_port 
	// free_port(ntm_mgr->nt_port_ctx, dport, 1);

	// if accepted client socket (created by `accept()`),
	// then Remove coresponding `nt_socket_t` in `accepted_conn_map` of `nt_listener_wrapper`
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


	DEBUG("handle NT_FIN_ACK success");

	return;

	FAIL: 

	
	return;
}



inline void handle_connect_ok_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {
	
	DEBUG("handle_connect_ok_msg success");

}

inline void handle_failure_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg) {

	DEBUG("handle_failure_msg success");

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
		goto FAIL;
	}

	// wait a while and destroy local ntm_conn
	usleep(1000);
	ntm_close_socket(ntm_conn->client_sock);

	DEBUG("handle_stop_msg success");

	return;

	FAIL: 
	
	return;

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

	DEBUG("handle_stop_confirm_msg success");

}


inline void send_connect_ok_msg(ntm_conn_t ntm_conn)
{
	assert(ntm_conn);

	ntm_sock_msg outgoing_msg;
	outgoing_msg.type = CONNECT_OK;
	ntm_send_tcp_msg(ntm_conn->client_sock, ((char *)&outgoing_msg),
					 sizeof(outgoing_msg));

	DEBUG("send CONNECT_OK message success");

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

		// accept connection
		client_sock = ntm_accept_tcp_conn(ntm_conn_ctx->server_sock);
		if (!client_sock)
		{
			if (!ntm_conn_ctx->running_signal)
			{
				ntm_close_socket(client_sock);
				break;
			}
			continue;
		}

		// start async recv thread to receive messages
		ntm_conn_t client_conn = (ntm_conn_t) calloc(1, sizeof(struct ntm_conn));
		client_conn->running_signal = true;
		client_conn->client_sock = client_sock;
		client_conn->sockfd = client_sock->socket_fd;
		client_conn->port = ntohs(client_sock->remote.sin_port);
		tmp_ip = inet_ntoa(client_sock->remote.sin_addr);
		client_conn->addrlen = strlen(tmp_ip);
		memcpy(client_conn->ip, tmp_ip, client_conn->addrlen);

		DEBUG("accept connection from %s:%d", tmp_ip, client_conn->port);

		// Push the new client connection into hash map
		// key: ip from remote ntb-monitor
		// value: struct ntm_conn
		Put(ntm_conn_ctx->conn_map, tmp_ip, client_conn);

		pthread_create(&client_conn->recv_thr, NULL, ntm_sock_recv_thread,
					   client_conn);

		send_connect_ok_msg(client_conn);
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
	ntm_conn->running_signal = true;
	int retval;

	while (ntm_conn->running_signal)
	{
		ntm_sock_msg incoming_msg;
		retval = ntm_recv_tcp_msg(ntm_conn->client_sock, (char *)&incoming_msg,
						 sizeof(incoming_msg));
		if(!retval == 0 && !ntm_conn->running_signal) {
			break;
		}
		if(!ntm_conn->running_signal) {
			break;
		}
		DEBUG("recv a message");
		retval = ntm_sock_handle_msg(ntm_conn, incoming_msg);
		if(retval == -1) {
			ERR("ntb-monitor failed and ready to exit");
			break;
		}
	}

	DEBUG("new ntm_sock receive thread end!!!");

	return NULL;
}

int ntm_sock_handle_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg)
{
	assert(ntm_conn);
	assert(ntm_conn->client_sock);

	int retval = 0;

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
		 *  iii. update nt_socket state as `WAIT_ESTABLISHED`, 
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
	case NT_CLIENT_SYN_ACK: {
		DEBUG("CLIENT SYN ACK message");

		/**
		 *  Specially for Server nt_socket located at ntb monitor
		 *	Receive `CLIENT SYN ACK` message from remote monitor.
		 *	1. check whether the CLIENT SYN ACK is valid
		 *	2. if valid:
		 *	 i. update corresponding client nt_socket state as `ESTABLISHED`
		 *	 ii. forward `CLIENT_SYN_ACK` nts_msg into corresponding client nt_socket via nts_shm
		 * 
		 */
		retval = handle_nt_client_syn_ack_msg(ntm_conn, msg);
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

	return retval;

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

static inline void handle_msg_nts_epoll_create(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_epoll_ctl(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_epoll_wait(ntm_manager_t ntm_mgr, ntm_msg msg);
static inline void handle_msg_nts_epoll_close(ntm_manager_t ntm_mgr, ntm_msg msg);


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
	DEBUG("push new nts shm conn into hashmap with sockid=%d, new_nts_shm_conn=%p.", new_nts_shm_conn->socket->sockid, new_nts_shm_conn);
	new_nts_shm_conn->socket->state = CLOSED;


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

	DEBUG("handle NTM_MSG_NEW_SOCK success");
	return;

	FAIL:
	DEBUG("FAIL code segment, free new_nts_shm_conn");
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

	DEBUG("handle_msg_nts_connect: sockid=%d", msg.sockid);
	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = NTS_MSG_DISPATCHED;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if(!nts_shm_conn) {
		ERR("nts_shm_conn not found");
		goto FAIL;
	}

	DEBUG("socket [sockid=%d] state is %d", msg.sockid, nts_shm_conn->socket->state);
	if (nts_shm_conn->socket->state != CLOSED 
			&& nts_shm_conn->socket->state != BOUND) {
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
	DEBUG("Check whether the target server ip is valid");
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
	if (msg.port < 0) {
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
	DEBUG("ready to connect remote ntb-based server with %s:%d, addrlen=%d", msg.address, msg.port, msg.addrlen);
	
	
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
		DEBUG("try to create connection with remote monitor of %s", msg.address);
		ntm_socket_t client_sock = ntm_sock_create();
		retval = ntm_connect_to_tcp_server(client_sock, NTM_LISTEN_PORT, msg.address);
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

		ntm_conn = (ntm_conn_t) calloc(1, sizeof(struct ntm_conn));

		ntm_conn->running_signal = true;
		ntm_conn->client_sock = client_sock;
		ntm_conn->sockfd = client_sock->socket_fd;
		ntm_conn->port = NTM_LISTEN_PORT;
		ntm_conn->addrlen = strlen(msg.address);
		memcpy(ntm_conn->ip, msg.address, ntm_conn->addrlen);

		DEBUG("setup connection to remote monitor %s:%d", msg.address, msg.port);

		pthread_create(&ntm_conn->recv_thr, NULL, 
						ntm_sock_recv_thread, ntm_conn);

		// cache the established ntm connection into hashmap
		// key: the ip address of remote monitor
		// value: struct ntm_conn
		Put(ntm_mgr->ntm_conn_ctx->conn_map, msg.address, ntm_conn);
		nts_shm_conn->ntm_conn = ntm_conn;
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
		DEBUG("allocate port=%d for `CLOSED` socket[sockid=%d]", port->port_id, msg.sockid);
	}


	// If NTB is supported: 
	// 1. dispatch `SYN` into target monitor via traditional TCP/IP transport;
	ntm_sock_msg syn_msg;
	syn_msg.type = NT_SYN;
	syn_msg.src_addr = nts_shm_conn->socket->saddr.sin_addr.s_addr;
	syn_msg.sport = nts_shm_conn->socket->saddr.sin_port;
	syn_msg.dst_addr = inet_addr(ntm_conn->ip);
	syn_msg.dport = htons(msg.port);
	int sport;
	sport = ntohs(syn_msg.sport);
	DEBUG("send NT_SYN with sport=%d, dport=%d, msg.port=%d, ntm_conn->port=%d", sport, msg.port, msg.port, ntm_conn->port);
	DEBUG("socked[sockid=%d] ready to ntm_send_tcp_msg send `NT_SYN` to remote monitor[%s]", msg.sockid, msg.address);
	retval = ntm_send_tcp_msg(ntm_conn->client_sock, (char*)&syn_msg, sizeof(syn_msg));
	if(retval <= 0) {
		ERR("ntm_send_tcp_msg send NT_SYN message failed");
		goto FAIL;
	}

	// 2. send `dispatched` message back to related nts app
	// return allocated `nt_port` in dispatched message
	response_msg.retval = 0;
	response_msg.port = sport;
	DEBUG("After dispatch `SYN`, send `dispatched` message [nt_port=%d] back to coresponding libnts [sockid=%d]", sport, msg.sockid);
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if(retval) {
		ERR("nts_shm_send failed for response to NTS_MSG_DISPATCHED");
		goto FAIL;
	}

	DEBUG("handle_msg_nts_connect success");
	return;

	FAIL:
	if(nts_shm_conn->socket->state == CLOSED) {
		if (port)
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

	DEBUG("handle_msg_nts_bind: sockid=%d, ip_addr=%s, port=%d", msg.sockid, msg.address, msg.port);
	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = NTS_MSG_BIND;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	DEBUG("locate nts_shm_conn=%p with sockid=%d", nts_shm_conn, msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		goto FAIL;
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

		goto FAIL;
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

		goto FAIL;
	}

	/**
	 * Check whether the bound port is valid and idle
	 */
	// check if the port in valid range
	if (msg.port < 0) // or other invalid range
	{
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_INVALID_PORT;
		retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		if (retval)
		{
			ERR("nts_shm_send failed for response to NT_ERR_INVALID_PORT");
		}

		goto FAIL;
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

		goto FAIL;
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

	// set the nt_socket->saddr
	nts_shm_conn->socket->saddr.sin_family = AF_INET;
	nts_shm_conn->socket->saddr.sin_port = htons(msg.port);
	nts_shm_conn->socket->saddr.sin_addr.s_addr = inet_addr(msg.address);
	// update the nt_socket->state as `BOUND`
	nts_shm_conn->socket->state = BOUND;

	/**
	 * send back success response to nts app
	 */
	response_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if (retval)
	{
		ERR("nts_shm_send failed for response to NTM_MSG_BIND");
		goto FAIL;
	}
	
	DEBUG("handle_msg_nts_bind success");
	return;

	FAIL: 

	return;

}

inline void handle_msg_nts_listen(ntm_manager_t ntm_mgr, ntm_msg msg)
{
	// assert(msg.msg_id > 0);
	// assert(msg.sockid > 0);

	nts_msg response_msg;
	nts_shm_conn_t nts_shm_conn;
	int retval;

	DEBUG("handle_msg_nts_listen: sockid=%d", msg.sockid);
	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = NTS_MSG_LISTEN;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		goto FAIL;
	}
	
	// listen() requires `BOUND` state first
	// if current socket_state is not BOUND, then error
	if (nts_shm_conn->socket->state != BOUND)
	{
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_REQUIRE_BIND_FIRST;
		ERR("listen() require bind() first");
		nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		goto FAIL;
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
	nt_listener_t new_listener;
	nt_listener_wrapper_t new_listener_wrapper;
	new_listener = (nt_listener_t) calloc(1, sizeof(struct nt_listener));
	new_listener_wrapper = (nt_listener_wrapper_t) calloc(1, sizeof(struct nt_listener_wrapper));

	// update the conresponding nt_socket: socket state, backlog
	new_listener->socket = nts_shm_conn->socket;
	new_listener->socket->socktype = NT_SOCK_LISTENER;
	new_listener->socket->state = LISTENING;
	new_listener->backlog = msg.backlog;
	new_listener->sockid = msg.sockid;
	
	new_listener_wrapper->listener = new_listener;
	new_listener_wrapper->port = nts_shm_conn->port;
	new_listener_wrapper->addrlen = nts_shm_conn->addrlen;
	memcpy(new_listener_wrapper->ip, nts_shm_conn->ip, nts_shm_conn->addrlen);
	new_listener_wrapper->nts_shm_conn = nts_shm_conn;
	new_listener_wrapper->accepted_conn_map = createHashMap(NULL, NULL);

	// setup backlog context
	sprintf(new_listener_wrapper->backlog_shmaddr, "backlog-%d", new_listener->sockid);
	new_listener_wrapper->backlog_shmlen = strlen(new_listener_wrapper->backlog_shmaddr);
	new_listener_wrapper->backlog_ctx = backlog_ntm(new_listener, 
							new_listener_wrapper->backlog_shmaddr, 
							new_listener_wrapper->backlog_shmlen, 
							new_listener->backlog);
	DEBUG("setup backlog context with backlog_shmaddr=%s, backlog_shmlen=%ld", 
			new_listener_wrapper->backlog_shmaddr, new_listener_wrapper->backlog_shmlen);  
	
	Put(ntm_mgr->nt_listener_ctx->listener_map, &new_listener_wrapper->port, new_listener_wrapper);
	DEBUG("Put new_listener_wrapper into `listener_map` indexed by nt_port=%d success", new_listener_wrapper->port);

	/**
	 * send back success response to nts app
	 */
	response_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
	if(retval){
		ERR("nts_shm_send failed for response to NTM_MSG_LISTEN");
		goto FAIL;
	}

	DEBUG("handle_msg_nts_listen success");
	return;

	FAIL: 

	return;

}

inline void handle_msg_nts_accept(ntm_manager_t ntm_mgr, ntm_msg msg)
{
	// assert(msg.msg_id > 0);
	// assert(msg.sockid > 0);

	nts_msg response_msg;
	nts_shm_conn_t nts_shm_conn;

	response_msg.msg_id = msg.msg_id;
	response_msg.msg_type = NTS_MSG_ACCEPT;
	response_msg.sockid = msg.sockid;
	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		goto FAIL;
	}

	// accept() requires `LISTENING` state first
	// if current socket_state is not LISTENING, then error
	if(nts_shm_conn->socket->state != LISTENING) {
		response_msg.retval = -1;
		response_msg.nt_errno = NT_ERR_REQUIRE_LISTEN_FIRST;
		ERR("accept() require listen() first");
		nts_shm_send(nts_shm_conn->nts_shm_ctx, &response_msg);
		goto FAIL;
	}


	DEBUG("handle_msg_nts_accept success");
	return;

	FAIL: 

	return;
}


inline void handle_msg_nts_accept_ack(ntm_manager_t ntm_mgr, ntm_msg msg) {
	nts_shm_conn_t nts_shm_conn;

	DEBUG("handle_msg_nts_accept_ack with sockid=%d", msg.sockid);

	// HashMapIterator iter = createHashMapIterator(ntm_mgr->nts_ctx->nts_shm_conn_map);
	// while(hasNextHashMapIterator(iter)) {
	// 	iter = nextHashMapIterator(iter);
	// 	nts_shm_conn = iter->entry->value;
	// 	DEBUG("{ key = %d, sockid = %d }", *(int *) iter->entry->key, nts_shm_conn->socket->sockid);
	// }
	// freeHashMapIterator(&iter);


	nts_shm_conn = (nts_shm_conn_t)Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	DEBUG("nts_shm_conn %p", nts_shm_conn);
	if (nts_shm_conn == NULL)
	{
		ERR("nts_shm_conn not found");
		goto FAIL;
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
		goto FAIL;
	}

	DEBUG("handle NTM_MSG_ACCEPT_ACK success");
	return;

	FAIL: 

	return;

}


inline void handle_msg_nts_close(ntm_manager_t ntm_mgr, ntm_msg msg)
{

	// Specially for NTB server socket (Listener nt_socket)
	/**
	 *  Receive the `NTM_MSG_CLOSE` message from libnts (mainly for ntb socket server).
	 * 	Destroy ntb server socket related resources.
	 * 	1. instruct ntb-proxy to destroy ntb connection/queue.
	 *  2. destroy the related resources in ntb monitor.
	 *  3. response the `SHUTDOWN Finished` message to nts app.
	 *  
	 */

	if(msg.msg_id <= 0){
		ERR("message id is not vaild");
		goto FAIL;
	}
	if(msg.sockid<=0){
		ERR("message socket id is not vaild");
		goto FAIL;
	}

	int retval;
	nts_shm_conn_t nts_shm_conn;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if(!nts_shm_conn){
		ERR("nts_shm_conn not found");
		goto FAIL;
	}

	if(nts_shm_conn->socket->socktype != NT_SOCK_LISTENER 
			&& nts_shm_conn->socket->state != LISTENING){
		ERR("listener nt_socket requires 'LISTENING' state.");
		goto FAIL;
	}

	// set socket state to WAIT_CLOSE
	nts_shm_conn->socket->state = WAIT_CLOSE;


	/**
	 * destroy nt_listener_wrapper, nt_listener_t, 
	 * 		backlog context, nt_port, nt_sock, nts_shm_conn
	 */
	// destroy nt_listener_wrapper
	nt_listener_wrapper_t listener_wrapper;
	listener_wrapper = (nt_listener_wrapper_t)
				Get(ntm_mgr->nt_listener_ctx->listener_map, &nts_shm_conn->port);

	// free backlog context
	backlog_ntm_close(listener_wrapper->backlog_ctx);

	// free/destroy listener_wrapper->accepted_conn_map
	//TODO: free the accepted client nt_socket in accepted_conn_map
	HashMapIterator iter;
	iter = createHashMapIterator(listener_wrapper->accepted_conn_map);
	while(hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		// nt_socket_t tmp_socket = (nt_socket_t)iter->entry->value;
		DEBUG("Destroy/free { key = %d }", *(int *) iter->entry->key);
	}
	freeHashMapIterator(&iter);

	Clear(listener_wrapper->accepted_conn_map);

	// free nt_listener_t 
	Remove(ntm_mgr->nt_listener_ctx->listener_map, &nts_shm_conn->port);
	free(listener_wrapper->listener);
	free(listener_wrapper);
	

	// Remove `nt_socket` from `ntm_mgr->port_sock_map`
	if (Exists(ntm_mgr->port_sock_map, &nts_shm_conn->port))
		Remove(ntm_mgr->port_sock_map, &nts_shm_conn->port);

	// free nt_port
	free_port(ntm_mgr->nt_port_ctx, nts_shm_conn->port, 1);


	nt_socket_t _socket;
	_socket = nts_shm_conn->socket;


	// overlap the nts shm communication with destroy nts_shm_context
	nts_msg resp_msg;
	resp_msg.msg_id = msg.msg_id;
	resp_msg.msg_type = NTS_MSG_CLOSE;
	resp_msg.sockid = msg.sockid;
	resp_msg.retval = 0;
	retval = nts_shm_send(nts_shm_conn->nts_shm_ctx, &resp_msg);
	if (retval)
	{
		ERR("nts_shm_send failed for response to NTM_MSG_CLOSE");
		goto FAIL;
	}
	usleep(50); // wait for 50us
	

	// close and destroy nts_shm_context_t in `nts_shm_conn`
	// free nts_shm_conn
	if (nts_shm_conn->nts_shm_ctx) {
		nts_shm_ntm_close(nts_shm_conn->nts_shm_ctx);
		nts_shm_destroy(nts_shm_conn->nts_shm_ctx);
	}

	if (Exists(ntm_mgr->nts_ctx->nts_shm_conn_map, &_socket->sockid)) {
		Remove(ntm_mgr->nts_ctx->nts_shm_conn_map, &_socket->sockid);
	}
	free(nts_shm_conn);


	// free nt_socket
	if(_socket) {
		free_socket(ntm_mgr->nt_sock_ctx, _socket->sockid, 1);
	}

	nts_shm_conn->socket->state = CLOSED;


	DEBUG("handle_msg_nts_close success");
	return;

	FAIL: 

	return;

}

inline void handle_msg_nts_fin(ntm_manager_t ntm_mgr, ntm_msg msg) {
	
	/**
	 * 1. get nts_shm_conn via msg.sockid
	 * 2. check whether socket state is `ESTABLISHED` or not
	 * 3. set socket state to `WAIT_CLOSE`
	 * 4. send `NT_FIN_ACK` message to remote ntm via traditional TCP transport
	 * 5. set socket state to `CLOSED`
	 * 6. destory client socket conn and nts_shm_conn
	 */
	if(msg.msg_id <= 0){
		ERR("message id is not vaild");
		goto FAIL;
	}
	if(msg.sockid<=0){
		ERR("message socket id is not vaild");
		goto FAIL;
	}

	int retval;
	nts_shm_conn_t nts_shm_conn;

	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if(!nts_shm_conn){
		ERR("nts_shm_conn not found");
		goto FAIL;
	}

	if(nts_shm_conn->socket->state != ESTABLISHED){
		ERR("`FIN` requires `ESTABLISHED` first");
		goto FAIL;
	}

	// set socket state to WAIT_CLOSE
	nts_shm_conn->socket->state = WAIT_CLOSE;

	ntm_conn_t ntm_conn;
	ntm_conn = nts_shm_conn->ntm_conn;
	DEBUG("msg.address='%s'.", msg.address);
	HashMapIterator iter;
	iter = createHashMapIterator(ntm_mgr->ntm_conn_ctx->conn_map);
	while(hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		// ntm_conn_t * tmp_ntm_conn = (ntm_conn_t *)iter->entry->value;
		DEBUG("{ key = %d }", *(int *) iter->entry->key);
	}
	freeHashMapIterator(&iter);

	if(!ntm_conn && Exists(ntm_mgr->ntm_conn_ctx->conn_map, msg.address)) {
		ntm_conn = (ntm_conn_t) Get(ntm_mgr->ntm_conn_ctx->conn_map, msg.address);
	}
	if(!ntm_conn){
		ERR("get ntm_conn failed");
		goto FAIL;
	}
	
	ntm_sock_msg sock_msg;
	sock_msg.src_addr = nts_shm_conn->socket->saddr.sin_addr.s_addr;
	sock_msg.sport = nts_shm_conn->socket->saddr.sin_port;
	sock_msg.dst_addr = nts_shm_conn->peer_sin_addr;
	sock_msg.dport = nts_shm_conn->peer_sin_port;
	sock_msg.type = NT_FIN_ACK;
	retval = ntm_send_tcp_msg(ntm_conn->client_sock, (char*)&sock_msg, sizeof(sock_msg));
	if(retval <= 0){
		ERR("ntm_send_tcp_msg send NT_FIN_ACK message failed");
		goto FAIL;
	}
	// set socket state to CLOSED
	nts_shm_conn->socket->state = CLOSED;
	
	destroy_client_nt_socket_conn_with_nts_shm_conn(nts_shm_conn);

	DEBUG("handle_msg_nts_fin success");
	return;

	FAIL: 

	return;

}

inline void handle_msg_err(ntm_manager_t ntm_mgr, ntm_msg msg)
{

	DEBUG("handle_msg_err success");

	return;
}

inline void handle_msg_nts_epoll_create(ntm_manager_t ntm_mgr, ntm_msg msg) {

	if (msg.msg_id < 0) {
		ERR("Invalid msg id for NTM_MSG_EPOLL_CREATE msg");
		return;
	}

	if (msg.nts_shm_addrlen < 0) {
		ERR("Invalid shm address length for epoll shm ringbuffer");
		return;
	}

	if (msg.io_queue_size < 0) {
		ERR("Invalid Epoll I/O queue size");
		return;
	}

	if (msg.sock_type != NT_SOCK_EPOLL) {
		ERR("Invalid Epoll Socket Type, require NT_SOCK_EPOLL type.");
		return;
	}

	/**
	 * 1. create epoll context and allocate EPOLL socket
	 * 2. create/init epoll shm ringbuffer
	 * 3. create SHM-based ready I/O queue
	 * 4. instruct ntp to create epoll_context
	 * 5. send back response msg to libnts
	 * 6. create HashMap to cache the epoll-enabled listener nt_socket_t
	 */

	// 1. create epoll context and allocate EPOLL socket
	epoll_context_t epoll_ctx;
	epoll_ctx = (epoll_context_t) calloc(1, sizeof(struct epoll_context));
	if (!epoll_ctx) {
		perror(MALLOC_ERR_MSG);
		ERR("calloc epoll_context_t failed");
		return;
	}
	 
	// allocate socket 
	DEBUG("allocate free NT_SOCK_EPOLL socket");
	epoll_ctx->socket = allocate_socket(ntm_mgr->nt_sock_ctx, msg.sock_type, 1);
	if (!epoll_ctx->socket) {
		free(epoll_ctx);
		ERR("allocate free NT_SOCK_EPOLL socket failed");
		return;
	}

	epoll_ctx->epoll_shmlen = msg.nts_shm_addrlen;
	memcpy(epoll_ctx->epoll_shmaddr, msg.nts_shm_name, epoll_ctx->epoll_shmlen);

	// 2. create/init epoll shm ringbuffer
	epoll_ctx->epoll_shm_ctx = epoll_shm();
	int retval = epoll_shm_connect(epoll_ctx->epoll_shm_ctx, 
			epoll_ctx->epoll_shmaddr, epoll_ctx->epoll_shmlen);
	if (retval) {
		free_socket(ntm_mgr->nt_sock_ctx, epoll_ctx->socket->sockid, 1);
		free(epoll_ctx);
		ERR("new epoll context connect epoll shm ringbuffer failed");
		return;
	}

	// 3. create SHM-based ready I/O queue
	epoll_ctx->io_queue_size = msg.io_queue_size;
	sprintf(epoll_ctx->io_queue_shmaddr, "%s%d", 
			EP_SHM_QUEUE_PREFIX, epoll_ctx->socket->sockid);
	epoll_ctx->io_queue_shmlen = strlen(epoll_ctx->io_queue_shmaddr);

	// TODO: Init SHM-based ready I/O queue
	epoll_ctx->ep_io_queue_ctx = ep_event_queue_create(
							epoll_ctx->io_queue_shmaddr, 
							epoll_ctx->io_queue_shmlen, 
							epoll_ctx->io_queue_size);
	if (!epoll_ctx->ep_io_queue_ctx) {
		epoll_shm_slave_close(epoll_ctx->epoll_shm_ctx);
		epoll_shm_destroy(epoll_ctx->epoll_shm_ctx);
		free_socket(ntm_mgr->nt_sock_ctx, epoll_ctx->socket->sockid, 1);
		free(epoll_ctx);
		ERR("epoll_shm_send failed for response to EPOLL_MSG_CREATE");
		return;
	}
	epoll_ctx->ep_io_queue = epoll_ctx->ep_io_queue_ctx->shm_queue;

	//TODO: 4. instruct ntp to create epoll_context
	epoll_msg req_ep_msg;
	req_ep_msg.id = msg.msg_id;
	req_ep_msg.sockid = epoll_ctx->socket->sockid;
	req_ep_msg.sock_type = NT_SOCK_EPOLL;
	req_ep_msg.msg_type = EPOLL_MSG_CREATE;
	req_ep_msg.io_queue_size = msg.io_queue_size;
	req_ep_msg.shm_addrlen = epoll_ctx->io_queue_shmlen;
	memcpy(req_ep_msg.shm_name, epoll_ctx->io_queue_shmaddr, epoll_ctx->io_queue_shmlen);
	retval = epoll_sem_shm_send(ntm_mgr->epoll_mgr->ntp_ep_recv_ctx, &req_ep_msg);
	if (retval) {
		ep_event_queue_free(epoll_ctx->ep_io_queue_ctx, true);
		epoll_shm_slave_close(epoll_ctx->epoll_shm_ctx);
		epoll_shm_destroy(epoll_ctx->epoll_shm_ctx);
		free_socket(ntm_mgr->nt_sock_ctx, epoll_ctx->socket->sockid, 1);
		free(epoll_ctx);
		ERR("epoll_sem_shm_send failed to send EPOLL_MSG_CREATE request to ntp");
		return;
	}

	//TODO: wait for the response epoll_msg from ntp
	epoll_msg resp_ep_msg;
	retval = epoll_sem_shm_recv(ntm_mgr->epoll_mgr->ntp_ep_send_ctx, &resp_ep_msg);
	if (retval || (retval == 0 && resp_ep_msg.retval != 0)) {
		ep_event_queue_free(epoll_ctx->ep_io_queue_ctx, true);
		epoll_shm_slave_close(epoll_ctx->epoll_shm_ctx);
		epoll_shm_destroy(epoll_ctx->epoll_shm_ctx);
		free_socket(ntm_mgr->nt_sock_ctx, epoll_ctx->socket->sockid, 1);
		free(epoll_ctx);
		ERR("epoll_sem_shm_recv failed to recv EPOLL_MSG_CREATE from ntp");
		return;
	}

	// 5. send back response msg to libnts
	epoll_msg resp_msg;
	resp_msg.id = msg.msg_id;
	resp_msg.retval = 0;
	resp_msg.sockid = epoll_ctx->socket->sockid;
	resp_msg.msg_type = EPOLL_MSG_CREATE;
	resp_msg.shm_addrlen = epoll_ctx->io_queue_shmlen;
	memcpy(resp_msg.shm_name, epoll_ctx->io_queue_shmaddr, epoll_ctx->io_queue_shmlen);
	retval = epoll_shm_send(epoll_ctx->epoll_shm_ctx, &resp_msg);
	if (retval) {
		ep_event_queue_free(epoll_ctx->ep_io_queue_ctx, true);
		epoll_shm_slave_close(epoll_ctx->epoll_shm_ctx);
		epoll_shm_destroy(epoll_ctx->epoll_shm_ctx);
		free_socket(ntm_mgr->nt_sock_ctx, epoll_ctx->socket->sockid, 1);
		free(epoll_ctx);
		ERR("epoll_shm_send failed for response to EPOLL_MSG_CREATE");
		return;
	}

	// 6. create HashMap to cache the epoll-enabled listener nt_socket_t
	epoll_ctx->ep_socket_map = createHashMap(NULL, NULL);

	// push new epoll context into hashmap
	Put(ntm_mgr->epoll_mgr->epoll_ctx_map, &epoll_ctx->socket->sockid, epoll_ctx);
	epoll_ctx->socket->state = CLOSED;

	DEBUG("handle EPOLL_MSG_CREATE success");
	return;
}

inline void handle_msg_nts_epoll_ctl(ntm_manager_t ntm_mgr, ntm_msg msg) {
	
	if (msg.msg_id < 0) {
		ERR("Invalid msg id for NTM_MSG_EPOLL_CTL msg");
		return;
	}

	if (msg.sockid < 0) {
		ERR("Invalid socket id for NTM_MSG_EPOLL_CTL msg");
		return;
	}

	if (msg.epid < 0) {
		ERR("Invalid epoll socket id for NTM_MSG_EPOLL_CTL msg");
		return;
	}

	if (msg.sock_type == NT_SOCK_UNUSED || msg.sock_type == NT_SOCK_EPOLL) {
		ERR("Invalid socket type for NTM_MSG_EPOLL_CTL msg");
		return;
	}
      
	if (msg.epoll_op != NTS_EPOLL_CTL_ADD && 
			msg.epoll_op != NTS_EPOLL_CTL_DEL && 
			msg.epoll_op != NTS_EPOLL_CTL_MOD) {
		ERR("Invalid epoll operation type for NTM_MSG_EPOLL_CTL msg");
		return;
	}	


	/**
	 *	1. get the corresponding epoll_context
	 *  2. get the sockid-corresponding socket, and check the validity
	 *  3. according to the epoll_op, take corresponding actions:
	 * 		case NTS_EPOLL_CTL_ADD: 
	 * 			socket->ep_data = msg.ep_data;
	 * 			socket->events = msg.events;
	 * 			instruct ntp to update socket-corresponding ntb-conn epoll state;
	 * 		case NTS_EPOLL_CTL_MOD:
	 * 			socket->ep_data = msg.ep_data;
	 * 			socket->events = msg.events;
	 * 			instruct ntp to update socket-corresponding ntb-conn epoll state;
	 * 		case NTS_EPOLL_CTL_DEL:
	 * 			socket->epoll = NTS_EPOLLNONE;
	 * 			instruct ntp to update socket-corresponding ntb-conn epoll state into NONE;
	 * 	4. recv the corresponding recv epoll_msg from ntp;
	 * 	5. send the response epoll_msg into libnts;
	 * 	
	 */

	// 1. get the corresponding epoll_context
	epoll_context_t epoll_ctx;
	epoll_ctx = (epoll_context_t) Get(ntm_mgr->epoll_mgr->epoll_ctx_map, &msg.epid);
	if (epoll_ctx == NULL) {
		ERR("Non-existing epoll_context for epoll_id[%d]", msg.epid);
		return;
	} 

	// 2. get the sockid-corresponding socket, and check the validity
	nts_shm_conn_t nts_shm_conn;
	nts_shm_conn = (nts_shm_conn_t) Get(ntm_mgr->nts_ctx->nts_shm_conn_map, &msg.sockid);
	if (!nts_shm_conn)
	{
		ERR("nts_shm_conn not found");
		return;
	}
	nt_socket_t socket = nts_shm_conn->socket;

	// 3. according to the epoll_op, take corresponding actions:
	int rc;
	epoll_msg resp_to_nts_msg;
	resp_to_nts_msg.id = msg.msg_id;
	resp_to_nts_msg.msg_type = EPOLL_MSG_CTL;
	resp_to_nts_msg.epoll_op = msg.epoll_op;

	if (msg.epoll_op == NTS_EPOLL_CTL_ADD) {
		if (socket->epoll) {
			resp_to_nts_msg.retval = -1;
			resp_to_nts_msg.nt_errno = EEXIST;
			rc = epoll_shm_send(epoll_ctx->epoll_shm_ctx, &resp_to_nts_msg);
			if (rc != 0) {
				ERR("epoll_shm_send failed to send response to EPOLL_MSG_CREATE");
			}
			return;
		}

		socket->ep_data = msg.ep_data;
		socket->epoll = msg.events;

	} else if (msg.epoll_op == NTS_EPOLL_CTL_MOD) {
		if (!socket->epoll) {
			resp_to_nts_msg.retval = -1;
			resp_to_nts_msg.nt_errno = ENOENT;
			rc = epoll_shm_send(epoll_ctx->epoll_shm_ctx, &resp_to_nts_msg);
			if (rc != 0) {
				ERR("epoll_shm_send failed to send EPOLL_MSG_CTL response to libnts");
			}
			return;
		}

		socket->ep_data = msg.ep_data;
		socket->epoll = msg.events;

	} else if (msg.epoll_op == NTS_EPOLL_CTL_DEL) {
		if (!socket->epoll) {
			resp_to_nts_msg.retval = -1;
			resp_to_nts_msg.nt_errno = ENOENT;
			rc = epoll_shm_send(epoll_ctx->epoll_shm_ctx, &resp_to_nts_msg);
			if (rc != 0) {
				ERR("epoll_shm_send failed to send EPOLL_MSG_CTL response to libnts");
			}
			return;
		}

		socket->epoll = NTS_EPOLLNONE;
	}

	if (socket->socktype == NT_SOCK_LISTENER) {  
		resp_to_nts_msg.retval = 0;
		rc = epoll_shm_send(epoll_ctx->epoll_shm_ctx, &resp_to_nts_msg);
		if (rc != 0) {
			ERR("epoll_shm_send failed to send EPOLL_MSG_CTL response to libnts");
		}
		if (msg.epoll_op == NTS_EPOLL_CTL_ADD) {
			Put(epoll_ctx->ep_socket_map, &socket->sockid, socket);
			nts_shm_conn->epoll_ctx = epoll_ctx;

		} else if (msg.epoll_op == NTS_EPOLL_CTL_DEL) {
			Remove(epoll_ctx->ep_socket_map, &socket->sockid);
			nts_shm_conn->epoll_ctx = NULL;

		}
		return;
	}

	// instruct ntp to update socket-corresponding ntb-conn epoll state
	epoll_msg req_to_ntp_msg;
	req_to_ntp_msg.id = msg.msg_id;
	req_to_ntp_msg.sockid = msg.sockid;
	req_to_ntp_msg.msg_type = EPOLL_MSG_CTL;
	req_to_ntp_msg.epid = msg.epid;
	req_to_ntp_msg.epoll_op = msg.epoll_op;
	if (msg.epoll_op == NTS_EPOLL_CTL_ADD) {
		req_to_ntp_msg.src_port = htons(nts_shm_conn->port);
		req_to_ntp_msg.dst_port = nts_shm_conn->peer_sin_port;
	}
	if (msg.epoll_op == NTS_EPOLL_CTL_ADD 
			|| msg.epoll_op == NTS_EPOLL_CTL_MOD) {
		req_to_ntp_msg.ep_data = msg.ep_data;
		req_to_ntp_msg.events = msg.events;
	}
	rc = epoll_sem_shm_send(ntm_mgr->epoll_mgr->ntp_ep_recv_ctx, &req_to_ntp_msg);
	if (rc) {
		ERR("epoll_sem_shm_send failed to send EPOLL_MSG_CTL request to ntp");
		return;
	}

	// 4. recv the corresponding recv epoll_msg from ntp
	// wait for the response epoll_msg from ntp
	epoll_msg resp_ep_msg;
	rc = epoll_sem_shm_recv(ntm_mgr->epoll_mgr->ntp_ep_send_ctx, &resp_ep_msg);
	if (rc || (rc == 0 && resp_ep_msg.retval != 0)) { 
		ERR("epoll_sem_shm_recv failed to recv EPOLL_MSG_CTL response from ntp");
		return;
	}

	// 5. send the response epoll_msg into libnts
	resp_to_nts_msg.retval = 0;
	rc = epoll_shm_send(epoll_ctx->epoll_shm_ctx, &resp_to_nts_msg);
	if (rc != 0) {
		ERR("epoll_shm_send failed to send EPOLL_MSG_CTL response to libnts");
	}
	return;
}

inline void handle_msg_nts_epoll_wait(ntm_manager_t ntm_mgr, ntm_msg msg) {

}

inline void handle_msg_nts_epoll_close(ntm_manager_t ntm_mgr, ntm_msg msg) {
	
	if (msg.msg_id < 0) {
		ERR("Invalid msg id for NTM_MSG_EPOLL_CTL msg");
		return;
	}

	if (msg.epid < 0 || msg.sock_type != NT_SOCK_EPOLL) {
		ERR("Invalid socket id or epoll socket type for NTM_MSG_EPOLL_CTL msg");
		return;
	}

	/**
	 * 1. get the corresponding epoll_context
	 * 2. get the sockid-corresponding socket, and check the validity
	 * 3. remove the epoll_fd-including listen socket within epoll_socket
	 * 4. instruct ntp to remove the epoll of all client socket within
	 * 		epoll_socket, destroy the corresponding epoll_context, and 
	 *  	disconnect the corresponding event queue.
	 * 5. destory shm-based event_queue thoroughly
	 * 6. send EPOLL_CLOSE ack to libnts
	 * 7. disconnect the epoll_shm_ring created by libnts 
	 * 8. destroy epoll_context and free epoll_socket
	 */

	// 1. get the corresponding epoll_context
	epoll_context_t epoll_ctx;
	epoll_ctx = (epoll_context_t) Get(ntm_mgr->epoll_mgr->epoll_ctx_map, &msg.epid);
	if (epoll_ctx == NULL) {
		ERR("Non-existing epoll_context for epoll_id[%d]", msg.epid);
		return;
	} 

	// 2. get the sockid-corresponding socket, and check the validity
	if (!epoll_ctx->socket || epoll_ctx->socket->sockid < 0) {
		ERR("Non-existing epoll_socket in epoll_context");
		Remove(ntm_mgr->epoll_mgr->epoll_ctx_map, &msg.epid);
		return;
	}

	// 3. remove the epoll_fd-including listen socket within epoll_socket
	HashMapIterator iter = createHashMapIterator(epoll_ctx->ep_socket_map);
	while (hasNextHashMapIterator(iter))
	{
		iter = nextHashMapIterator(iter);
		nt_socket_t listen_sock = (nt_socket_t) (iter->entry->value);
		listen_sock->epoll = NTS_EPOLLNONE;
	}
	freeHashMapIterator(&iter);
	Clear(epoll_ctx->ep_socket_map);
	epoll_ctx->ep_socket_map = NULL;


	// 4. instruct ntp to remove the epoll of all client socket within
	// 	epoll_socket, destroy the corresponding epoll_context, and 
	//  disconnect the corresponding event queue.
	epoll_msg req_ep_msg;
	req_ep_msg.id = msg.msg_id;
	req_ep_msg.sock_type = NT_SOCK_EPOLL;
	req_ep_msg.msg_type = EPOLL_MSG_CLOSE;
	req_ep_msg.epid = msg.epid;
	int rc;
	rc = epoll_sem_shm_send(ntm_mgr->epoll_mgr->ntp_ep_recv_ctx, &req_ep_msg);
	if (rc) {
		ERR("epoll_sem_shm_send failed to send EPOLL_MSG_CLOSE request to ntp");
		return;
	}

	// recv the corresponding epoll_msg from ntp
	epoll_msg resp_ep_msg;
	rc = epoll_sem_shm_recv(ntm_mgr->epoll_mgr->ntp_ep_send_ctx, &resp_ep_msg);
	if (rc || (rc == 0 && resp_ep_msg.retval != 0)) {
		ERR("epoll_sem_shm_recv failed to recv EPOLL_MSG_CLOSE response from ntp");
		return;
	}

	// 5. destory shm-based event_queue thoroughly
	ep_event_queue_free(epoll_ctx->ep_io_queue_ctx, true);
	epoll_ctx->ep_io_queue = NULL;

	// 6. send EPOLL_CLOSE ack to libnts
	epoll_msg resp_msg;
	resp_msg.id = msg.msg_id;
	resp_msg.retval = 0;
	resp_msg.epid = msg.epid;
	resp_msg.msg_type = EPOLL_MSG_CLOSE;
	rc = epoll_shm_send(epoll_ctx->epoll_shm_ctx, &resp_msg);
	// if epoll_shm_send failed, 
	// 	continue to destroy the epoll context.
	if (rc) {
		ERR("epoll_shm_send failed for response to EPOLL_MSG_CLOSE");
	}

	// 7. disconnect the epoll_shm_ring created by libnts 
	epoll_shm_slave_close(epoll_ctx->epoll_shm_ctx);
	epoll_shm_destroy(epoll_ctx->epoll_shm_ctx);

	// 8. destroy epoll_context and free epoll_socket
	Remove(ntm_mgr->epoll_mgr->epoll_ctx_map, &epoll_ctx->socket->sockid);	
	free_socket(ntm_mgr->nt_sock_ctx, epoll_ctx->socket->sockid, 1);
	free(epoll_ctx);
	epoll_ctx = NULL;
}


void *nts_shm_recv_thread(void *args)
{
	assert(ntm_mgr);
	assert(ntm_mgr->nts_ctx->shm_recv_ctx);

	DEBUG("nts_shm_recv_thread ready...");
	ntm_nts_context_t nts_ctx;
	nts_ctx = ntm_mgr->nts_ctx;
	
	ntm_msg recv_msg;
	nts_ctx->shm_recv_signal = 1; // if 1, running; else stop

	while (nts_ctx->shm_recv_signal)
	{
		int retval;
		retval = ntm_shm_recv(nts_ctx->shm_recv_ctx, &recv_msg);
		if(nts_ctx->shm_recv_signal == 0) 
			break;
		if (retval == 0)
		{
			DEBUG("receive a message");
			nts_shm_handle_msg(ntm_mgr, recv_msg);
		}
		else
		{
			ERR("failed to receive a message");
			break;
		}
	}

	DEBUG("nts_shm_recv_thread end!");

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
		DEBUG("handle NTM_MSG_ACCEPT_ACK");

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
	else if (msg.msg_type & NTM_MSG_EPOLL_CREATE)
	{
		DEBUG("handle NTM_MSG_EPOLL_CREATE");
	}
	else if (msg.msg_type & NTM_MSG_EPOLL_CTL)
	{
		DEBUG("handle NTM_MSG_EPOLL_CTL");
	}
	else if (msg.msg_type & NTM_MSG_EPOLL_CLOSE)
	{
		DEBUG("handle NTM_MSG_EPOLL_CLOSE");
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

	usleep(10);
	ntm_close_socket(client_sock);
	return true;

}

inline void destroy_client_nt_socket_conn(nt_socket_t client_socket, 
				nts_shm_conn_t nts_shm_conn, int bound_port) {
	assert(ntm_mgr);
	assert(client_socket);
	assert(client_socket->sockid > 0);
	assert(nts_shm_conn);
	assert(bound_port >= 0);

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