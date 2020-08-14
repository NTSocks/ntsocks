/*
 * <p>Title: nts_api.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 9, 2019 
 * @version 1.0
 */


#include <assert.h>
#include <errno.h>

#include "nts_api.h"
#include "nts_config.h"
#include "nt_log.h"
#include "ntp2nts_shm.h"
#include "ntm_shm.h"
#include "nts_shm.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);


static inline int handle_ntp_fin_msg(nt_sock_context_t nt_sock_ctx, int sockid);


static void * ntm_recv_thread(void *arg);

static void * ntm_send_thread(void *arg);

static void * ntp_recv_thread(void *arg);

static void * ntp_send_thread(void *arg);


// if is `NTP_NTS_MSG_FIN`, then set the socket state to `WAIT_FIN` 
// 	    and start to passively close nt_socket
// two solution to destroy client socket-related resources in libnts and ntm
// solution 1: nts send `NTM_MSG_FIN` message into ntm, then: 
// 			set socket state to `CLOSED`, destroy local resources, 
//			nts_shm_ntm_close()/nts_shm_close()[??] to close nts_shm.
// solution 2: nts send `NTM_MSG_FIN` message into ntm, then:
//			set socket state to `WAIT_FIN`, wait for the response ACK from ntm,
//			set socket state to `CLOSED`, destroy local resources, 
//			nts_shm_close() to unlink nts_shm.
// To the end, we determine to use solution 1.
inline int handle_ntp_fin_msg(nt_sock_context_t nt_sock_ctx, int sockid) {
	assert(nt_sock_ctx);

	nt_sock_ctx->socket->state = WAIT_FIN;
		
	int retval;
	ntm_msg ntm_outgoing_msg;
	ntm_outgoing_msg.msg_type = NTM_MSG_FIN;
	ntm_outgoing_msg.sockid = sockid;
	retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &ntm_outgoing_msg);
	if (retval == -1) {
		ERR("ntm_shm_send NTM_MSG_FIN failed. ");
		return -1;
	}

	nt_sock_ctx->socket->state = CLOSED;
	
	// destroy socket related resources
	// i.e., ntp_send_ctx, ntp_recv_ctx, nts_shm_ctx, socket
	DEBUG("destroy nt_socket-related resources.");
	if(nt_sock_ctx->ntp_send_ctx) {
		ntp_shm_close(nt_sock_ctx->ntp_send_ctx);
		ntp_shm_destroy(nt_sock_ctx->ntp_send_ctx);
	}
	if(nt_sock_ctx->ntp_recv_ctx) {
		ntp_shm_close(nt_sock_ctx->ntp_recv_ctx);
		ntp_shm_destroy(nt_sock_ctx->ntp_recv_ctx);
	}

	// if (nt_sock_ctx->nts_shm_ctx) {
	// 	nts_shm_close(nt_sock_ctx->nts_shm_ctx);
	// 	nts_shm_destroy(nt_sock_ctx->nts_shm_ctx);
	// }

	// free(nt_sock_ctx->socket);

	DEBUG("handle_ntp_fin_msg success.");

	return 0;

}


//int nts_init(int argc, char * const argv[]) {
//
//	return 0;
//}

void * ntm_recv_thread(void *arg) {
	
	return 0;
}

void * ntm_send_thread(void *arg) {
	assert(nts_ctx->ntm_ctx->shm_send_ctx);

	DEBUG("start to send requests to ntb monitor ...");

	// int i;
	// for (i = 0; i < 10; i++) {
	// 	ntm_msg *msg = (struct ntm_msg) calloc(1, sizeof(struct ntm_msg));
	// 	msg->msg_type = NTM_MSG_INIT;
	// 	msg->addrlen = sizeof(MSG_FROM_NTS);
	// 	ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, msg);
	// }
	ntm_msg msg;
	msg.msg_type = NTM_MSG_INIT;
	msg.msg_id = 1;

	ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &msg);

	DEBUG("ntm_send_thread end!");

	return 0;
}

void * ntp_recv_thread(void *arg) {

	return 0;
}

void * ntp_send_thread(void *arg) {

	return 0;
}



int nts_init(const char *config_file) {
	assert(config_file);

	nts_context_init(config_file);
	nts_ctx->init_flag = 1;

	return 0;
}

void nts_destroy() {
	nts_context_destroy();
}

int nts_setconf(struct nts_config *conf) {

	return 0;
}

int nts_get_conf(struct nts_config *conf) {

	return 0;
}


/* POSIX-LIKE api begin */

int nts_fcntl(int fd, int cmd, ...) {

	return 0;
}

int nts_sysctl(const int *name, u_int namelen, void *oldp, size_t *oldlenp,
    const void *newp, size_t newlen) {
	return 0;
}

int nts_ioctl(int fd, unsigned long request, ...) {

	return 0;
}

int nts_socket(int domain, int type, int protocol) {

	DEBUG("entering nts_socket()...");
	assert(nts_ctx);

	// ntm_send_thread(NULL);

	/**
	 * 1. generate nts_shm-uuid shm name
	 * 2. init/create nts_socket-corresponding nts_shm_ring 
	 * 	  to receive response message from ntb monitor
	 * 3. pack the `NTM_MSG_NEW_SOCK` ntm_msg and `ntm_shm_send()` the message into ntm
	 * 4. poll or wait for the response message from ntm
	 * 5. parse the response nts_msg with nt_socket_id, if success, create one nt_sock_context, nt_socket, 
	 * 		and setup the nt_sock_context
	 * 6. push `nt_sock_context` into `HashMap nt_sock_map`, then return sockid
	 */
	
	nt_sock_context_t nt_sock_ctx;
	nt_sock_ctx = (nt_sock_context_t)calloc(1, sizeof(struct nt_sock_context));
	if(!nt_sock_ctx)
		goto FAIL;
	
	nt_sock_ctx->ntm_msg_id = 1;

	// generate nts_shm-uuid shm name
	int retval;
	retval = generate_nts_shmname(nt_sock_ctx->nts_shmaddr);
	if (retval == -1) {
		ERR("generate nts_shm-uuid shm name failed");
		goto FAIL;
	}
	nt_sock_ctx->nts_shmlen = strlen(nt_sock_ctx->nts_shmaddr);

	// init/create nts_socket-coresponding nts_shm_ring 
	nt_sock_ctx->nts_shm_ctx = nts_shm();
	nts_shm_accept(nt_sock_ctx->nts_shm_ctx, 
				nt_sock_ctx->nts_shmaddr, nt_sock_ctx->nts_shmlen);
	
	// pack the `NTM_MSG_NEW_SOCK` ntm_msg and send the message into ntm
	DEBUG("nts_shmaddr=%s, nts_shmlen=%d", nt_sock_ctx->nts_shmaddr, nt_sock_ctx->nts_shmlen);
	ntm_msg outgoing_msg;
	outgoing_msg.msg_id = nt_sock_ctx->ntm_msg_id;
	outgoing_msg.msg_type = NTM_MSG_NEW_SOCK;
	outgoing_msg.nts_shm_addrlen = nt_sock_ctx->nts_shmlen;
	memcpy(outgoing_msg.nts_shm_name, nt_sock_ctx->nts_shmaddr, nt_sock_ctx->nts_shmlen);
	outgoing_msg.domain = domain;
	outgoing_msg.sock_type = type;
	outgoing_msg.protocol = protocol;
	retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &outgoing_msg);
	if(retval) {
		ERR("ntm_shm_send NTM_MSG_NEW_SOCK message failed");
		goto FAIL;
	}

	// poll or wait for the response message from ntm
	nts_msg incoming_msg;
	retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	while(retval) {
		sched_yield();
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	}

	// parse the response nts_msg with nt_socket_id
	if (incoming_msg.msg_id != outgoing_msg.msg_id) {
		ERR("invalid message id for NEW_SOCKET response");
		goto FAIL;
	}
	if (incoming_msg.retval == -1) {
		ERR("NEW_SOCKET failed");
		goto FAIL;
	}

	nt_socket_t socket;
	socket = (nt_socket_t)calloc(1, sizeof(struct nt_socket));
	socket->sockid = incoming_msg.sockid;
	socket->socktype = type;
	socket->state = CLOSED;
	nt_sock_ctx->socket = socket;
	nt_sock_ctx->ntm_msg_id ++;

	// push `nt_sock_context` into `HashMap nt_sock_map`, then return sockid
	Put(nts_ctx->nt_sock_map, &socket->sockid, nt_sock_ctx);
	DEBUG("nts_socket() pass with sockid=%d", socket->sockid);

	return socket->sockid;

	FAIL: 
	if(socket) {
		free(socket);
	}

	if(nt_sock_ctx->nts_shm_ctx) {
		if (nt_sock_ctx->nts_shm_ctx->shm_stat == NTM_SHM_READY) {
			nts_shm_close(nt_sock_ctx->nts_shm_ctx);
		}
		nts_shm_destroy(nt_sock_ctx->nts_shm_ctx);
	}

	if(nt_sock_ctx) {
		free(nt_sock_ctx);
	}

	return -1;

}

int nts_setsockopt(int sockid, int level, int optname, const void *optval,
    socklen_t optlen) {
	DEBUG("nts_setsockopt() start...");
	assert(nts_ctx);

	DEBUG("nts_setsockopt() success");
	return 0;
}

int nts_getsockopt(int sockid, int level, int optname, void *optval,
    socklen_t *optlen) {
	DEBUG("nts_getsockopt() start...");
	assert(nts_ctx);

	DEBUG("nts_getsockopt() success");
	return 0;
}

int nts_listen(int sockid, int backlog) {
	DEBUG("nts_listen() start...");
	assert(nts_ctx);
	assert(sockid >= 0);
	assert(backlog > 0);

	/**
	 * 1. pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	 * 3. pack the `NTM_MSG_LISTEN` ntm_msg and `ntm_shm_send()` the message into ntm
	 * 4. poll or wait for the response message from ntm
	 * 5. parse the response nts_msg with nt_socket_id, if success, return 0, else return -1
	 */
	nt_sock_context_t nt_sock_ctx;
	nt_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx)
		goto FAIL;

	if(!nt_sock_ctx->socket || nt_sock_ctx->socket->state != BOUND) {
		ERR("non-existing socket [sockid: %d], or the socket state is not `BOUND`. ", sockid);
		goto FAIL;
	}

	int retval;
	
	// pack the `NTM_MSG_LISTEN` ntm_msg and `ntm_shm_send()` the message into ntm
	ntm_msg outgoing_msg;
	outgoing_msg.msg_id = nt_sock_ctx->ntm_msg_id;
	outgoing_msg.msg_type = NTM_MSG_LISTEN;
	outgoing_msg.sockid = sockid;
	outgoing_msg.backlog = backlog;
	retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &outgoing_msg);
	if(retval){
		ERR("ntm_shm_send NTM_MSG_LISTEN message failed");
		goto FAIL;
	}

	//poll or wait for the response message from ntm
	nts_msg incoming_msg;
	retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	while(retval){
		sched_yield();
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	}

	// parse the response nts_msg with nt_socket_id
	if(incoming_msg.msg_id != outgoing_msg.msg_id){
		ERR("invalid message id for LISTEN response");
		goto FAIL;
	}
	if(incoming_msg.retval == -1){
		ERR("SOCKET LISTEN failed");
		goto FAIL;
	}

	// if the response message is valid
	// i. update the socket state, socktype
	// ii. create/get backlog context 
	nt_sock_ctx->socket->state = LISTENING;
	nt_sock_ctx->socket->socktype = NT_SOCK_LISTENER;

	DEBUG("create/get backlog context ");
	char backlog_shmaddr[30]; // 30 is the maximum size of backlog shm addr
	int backlog_shmlen;
	sprintf(backlog_shmaddr, "backlog-%d", sockid);
	backlog_shmlen = strlen(backlog_shmaddr);
	nt_sock_ctx->backlog_ctx = backlog_nts(nt_sock_ctx->socket, backlog_shmaddr, backlog_shmlen, backlog);
	DEBUG("setup backlog context with backlog_shmaddr=%s, backlog_shmlen=%d", backlog_shmaddr, backlog_shmlen);

	nt_sock_ctx->ntm_msg_id++;
	DEBUG("nts_listen() success");
	return 0;

	FAIL:
	return -1;
}

int nts_bind(int sockid, const struct sockaddr *addr, socklen_t addrlen){
	DEBUG("nts_bind() start...");
	assert(nts_ctx);
	assert(sockid >= 0);
	assert(addrlen > 0);
	assert(addr);

	/**
	 * 1. pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	 * 2. pack the `NTM_MSG_BIND` ntm_msg and `ntm_shm_send()` the message into ntm
	 * 3. poll or wait for the response message from ntm
	 * 4. parse the response nts_msg with nt_socket_id, if success, 
	 */
	int retval;

	nt_sock_context_t nt_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx) {
		ERR("sockid [%d] has no coresponding nt_sock_context_t.", sockid);
		goto FAIL;
	}
		

	// check whether the socket state is valid or not
	if(!nt_sock_ctx->socket || nt_sock_ctx->socket->state != CLOSED) {
		ERR("non-existing socket [sockid: %d], or the socket state is not `CLOSED`. ", sockid);
		goto FAIL;
	}
	
	// pack the `NTM_MSG_BIND` ntm_msg and `ntm_shm_send()` the message into ntm
	struct sockaddr_in *sock = (struct sockaddr_in*) addr;
	ntm_msg outgoing_msg;
	outgoing_msg.msg_id = nt_sock_ctx->ntm_msg_id;
	outgoing_msg.msg_type = NTM_MSG_BIND;
	outgoing_msg.sockid = sockid;
	outgoing_msg.port = ntohs(sock->sin_port);
	struct in_addr in = sock->sin_addr;
	inet_ntop(AF_INET, &in, outgoing_msg.address, sizeof(outgoing_msg.address));
	outgoing_msg.addrlen = strlen(outgoing_msg.address);
	DEBUG("the bind ip:port = %s:%d, with addrlen=%d", outgoing_msg.address, outgoing_msg.port, outgoing_msg.addrlen);
	

	//TODO:delete
	// retval = ip_is_vaild(outgoing_msg.address);
	// if(retval){
	// 	ERR("the ip addr is not vaild. the current ip is %s, while the vaild ip is %s", outgoing_msg.address, NTS_CONFIG.nt_host);
	// 	goto FAIL;
	// }
	DEBUG("ntm_shm_send: msg_id=%d, msg_type=%d, sockid=%d", outgoing_msg.msg_id, outgoing_msg.msg_type, outgoing_msg.sockid);
	retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &outgoing_msg);
	if(retval){
		ERR("ntm_shm_send NTM_MSG_BIND message failed");
		goto FAIL;
	}

	//poll or wait for the response message from ntm
	nts_msg incoming_msg;
	retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	while(retval){
		sched_yield();
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	}

	// parse the response nts_msg with nt_socket_id
	if(incoming_msg.msg_id != outgoing_msg.msg_id){
		ERR("invalid message id for BIND response");
		goto FAIL;
	}
	if(incoming_msg.retval == -1){
		ERR("SOCKET BIND failed");
		goto FAIL;
	}

	// if the response message is valid, update the socket state
	// set the nt_socket->saddr
	nt_sock_ctx->socket->saddr.sin_family = AF_INET;
	nt_sock_ctx->socket->saddr.sin_port = sock->sin_port;
	nt_sock_ctx->socket->saddr.sin_addr = sock->sin_addr;
	nt_sock_ctx->socket->state = BOUND;
	nt_sock_ctx->ntm_msg_id++;

	DEBUG("nts_bind() success");
	return 0;

	FAIL:
	return -1;
}

int nts_accept(int sockid, const struct sockaddr *addr, socklen_t *addrlen) {
	DEBUG("nts_accept start...");
	assert(nts_ctx);
	assert(sockid >= 0);

	/**
	 * 1. get the coressponding nt_sock_context via nt_socket_id
	 * 2. check whether the specified sockid is a listener socket via check socket state
	 * 3. check whether the backlog is ready ?
	 * 4. pop a `WAIT_ESTABLISHED` client socket, set socket type as `NT_SOCK_PIPE`,
	 * 		set the listener_socket as current listener socket
	 * 5. create coresponding nt_sock_context, init/create the nts shm queue, 
	 * 		set its `listener_socket`, init/create ntp_send_ctx/ntp_recv_ctx,
	 * 		push the `nt_sock_context` into HashMap `nt_sock_map` in nts_context
	 * 5.1 wait `NTS_MSG_CLIENT_SYN_ACK` nts_msg from local monitor
	 * 6. send the nts_shmaddr to ntb monitor via ntm_shm queue
	 * 7. check whether the listener socket state is `CLOSED`
	 * 8. return `ESTABLISHED` client socket id, sockaddr
	 */

	// 1. get the coressponding nt_sock_context via nt_socket_id
	nt_sock_context_t nt_sock_ctx;
	nt_sock_ctx = (nt_sock_context_t)Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx || !nt_sock_ctx->socket) {
		ERR("Non-existing sockid or Invalid sockid");
		goto FAIL;
	}

	// 2. check whether the specified sockid is a listener socket via check socket state
	if (nt_sock_ctx->socket->socktype != NT_SOCK_LISTENER) {
		ERR("the specified sockid is not a passive listener socket");
		goto FAIL;
	}

	if (nt_sock_ctx->socket->state != LISTENING) {
		ERR("the specified listener sockid is not in `LISTENING` state");
		goto FAIL;
	}

	// 3. check whether the backlog is ready ?
	if (!nt_sock_ctx->backlog_ctx || 
			nt_sock_ctx->backlog_ctx->backlog_stat != BACKLOG_READY) {
		ERR("the backlog of the listener socket is non-existing or not ready.");
		goto FAIL;
	}

	// 4. pop a `WAIT_ESTABLISHED` client socket, set socket type as `NT_SOCK_PIPE`,
	// 		set the listener_socket as current listener socket
	int retval;
	nt_socket_t client_socket;
	client_socket = (nt_socket_t)calloc(1, sizeof(struct nt_socket));
	retval = backlog_pop(nt_sock_ctx->backlog_ctx, client_socket);
	while (retval == -1)
	{
		sched_yield();
		retval = backlog_pop(nt_sock_ctx->backlog_ctx, client_socket);
	}
	DEBUG("accept one client nt_socket with sockid=%d", client_socket->sockid);


	// 5. create coresponding nt_sock_context, init/create the nts shm queue, 
	// set its `listener_socket`, init/create ntp_send_ctx/ntp_recv_ctx,
	// push the `nt_sock_context` into HashMap `nt_sock_map` in nts_context
	// tell the local ntb monitor the coresponding nts shm address via `ntm_shm_send`
	nt_sock_context_t client_sock_ctx;
	client_sock_ctx = (nt_sock_context_t)calloc(1, sizeof(struct nt_sock_context));
	if(!nt_sock_ctx)
		goto FAIL;

	client_sock_ctx->ntm_msg_id = 1;
	client_sock_ctx->socket = client_socket;
	// set its `listener_socket`
	client_sock_ctx->listener_socket = nt_sock_ctx->socket;

	// generate nts_shm-uuid shm name
	retval = generate_nts_shmname(client_sock_ctx->nts_shmaddr);
	if(retval == -1) {
		ERR("generate nts_shm-uuid shm name failed");
		goto FAIL;
	}
	client_sock_ctx->nts_shmlen = strlen(client_sock_ctx->nts_shmaddr);

	// init/create nts_socket-coresponding nts_shm_ring
	client_sock_ctx->nts_shm_ctx = nts_shm();
	nts_shm_accept(client_sock_ctx->nts_shm_ctx, 
				client_sock_ctx->nts_shmaddr, client_sock_ctx->nts_shmlen);

	
	// get the shm name for ntp_send_ctx/ntp_recv_ctx: 
	// `[local listener_port]-[remote client_socket_port]-r` for ntp recv shm name
	// `[local listener_port]-[remote client_socket_port]-s` for ntp send shm name
	char recv_shmaddr[SHM_NAME_LEN], send_shmaddr[SHM_NAME_LEN];
	int listener_port, client_port;
	listener_port = parse_sockaddr_port(&nt_sock_ctx->socket->saddr);	
	client_port = parse_sockaddr_port(&client_socket->saddr);
	sprintf(recv_shmaddr, "%d-%d-r", listener_port, client_port);
	sprintf(send_shmaddr, "%d-%d-s", listener_port, client_port);
	DEBUG("ntp send shm addr='%s', ntp recv shm addr='%s'", send_shmaddr, recv_shmaddr);

	// TODO:init/create ntp_send_ctx/ntp_recv_ctx
	client_sock_ctx->ntp_recv_ctx = ntp_shm();
	client_sock_ctx->ntp_send_ctx = ntp_shm();
	ntp_shm_connect(client_sock_ctx->ntp_recv_ctx, recv_shmaddr, strlen(recv_shmaddr));
	ntp_shm_connect(client_sock_ctx->ntp_send_ctx, send_shmaddr, strlen(send_shmaddr));
	
	// push the `nt_sock_context` into HashMap `nt_sock_map` in nts_context
	Put(nts_ctx->nt_sock_map, &client_socket->sockid, client_sock_ctx);

	// 5.1 wait `NTS_MSG_CLIENT_SYN_ACK` nts_msg from local monitor
	DEBUG("wait `NTS_MSG_CLIENT_SYN_ACK` nts_msg from local monitor");
	nts_msg syn_ack_msg;
	retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &syn_ack_msg);
	while(retval){
		sched_yield();
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &syn_ack_msg);
	}

	if(syn_ack_msg.retval == -1){
		ERR("SOCKET CONNECT failed");
		goto FAIL;
	}
	if(syn_ack_msg.msg_type != NTS_MSG_CLIENT_SYN_ACK) {
		ERR("expect NTS_MSG_CLIENT_SYN_ACK msg");
		goto FAIL;
	}
	//TODO: maybe disable this judgement
	if (syn_ack_msg.sockid != client_socket->sockid) {
		ERR("The NTS_MSG_CLIENT_SYN_ACK msg [sockid=%d] is not client sockid [%d]", syn_ack_msg.sockid, client_socket->sockid);
		goto FAIL;
	}
	// if NTS_MSG_CLIENT_SYN_ACK, update client_socket state from 'WAIT_ESTABLISHED' to `ESTABLISHED`
	client_socket->state = ESTABLISHED;
	INFO("recv `NTS_MSG_CLIENT_SYN_ACK` nts_msg success");


	// 6. send the nts_shmaddr to local ntb monitor via ntm_shm queue
	// pack `NTM_MSG_ACCEPT_ACK` ntm_msg and sent it into ntb monitor
	ntm_msg outgoing_msg;
	outgoing_msg.msg_id = client_sock_ctx->ntm_msg_id;
	outgoing_msg.sockid = client_socket->sockid;
	outgoing_msg.msg_type = NTM_MSG_ACCEPT_ACK;
	outgoing_msg.nts_shm_addrlen = client_sock_ctx->nts_shmlen;
	memcpy(outgoing_msg.nts_shm_name, client_sock_ctx->nts_shmaddr, client_sock_ctx->nts_shmlen);
	DEBUG("ntm_shm_send NTM_MSG_ACCEPT_ACK with sockid=%d", client_socket->sockid);
	retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &outgoing_msg);
	if(retval) {
		ERR("ntm_shm_send NTM_MSG_ACCEPT_ACK message failed");
		goto FAIL;
	}


	// 7. check whether the listener socket state is `CLOSED`
	if (nt_sock_ctx->socket->state == CLOSED) {
		ERR("the specified listener sockid is not in `LISTENING` state");
		goto FAIL;
	}


	// 8. return `ESTABLISHED` client socket id, sockaddr
	if (addr && addrlen) {
		// TODO: if `addr` is required, copy into addr and return
		DEBUG("copy the IP:Port of accepted client socket into param `addr`. ");
	}


	DEBUG("nts_accept success");
	return client_socket->sockid;

	FAIL: 
	if(client_socket) {
		free(client_socket);
	}

	if(client_sock_ctx->nts_shm_ctx) {
		if (client_sock_ctx->nts_shm_ctx->shm_stat == NTM_SHM_READY) {
			nts_shm_close(client_sock_ctx->nts_shm_ctx);
		}
		nts_shm_destroy(client_sock_ctx->nts_shm_ctx);
	}

	if(client_sock_ctx) {
		free(client_sock_ctx);
	}

	return -1;

}

int nts_connect(int sockid, const struct sockaddr *name, socklen_t namelen) {
	DEBUG("nts_connect start...");
	assert(nts_ctx);
	assert(sockid >= 0);
	assert(namelen > 0);
	assert(name);

	/**
	 * 1. get `nt_sock_context` from `HashMap nt_sock_map` via sockid. 
	 * 2. parse the sockaddr to get ip and port
	 * 3. check whether the ip addr is valid or not
	 * 4. pack the `NTM_MSG_CONNECT` ntm_msg and `ntm_shm_send()` the message into ntm
	 * 5. poll or wait for the response message [NTS_MSG_DISPATCHED msg] from ntm
	 * 6. poll or wait for the response message [NTS_MSG_ESTABLISH msg] from ntm
	 * 7. parse the response nts_msg with nt_socket_id, 
	 * 		if success, change socket state [] and socket type.
	 * 		init/create ntp_send_ctx/ntp_recv_ctx according to 
	 * 		[local_src_port]-[remote_dst_port]-[r/s] in corresponding nt_sock_context_t.
	 * 8. check incoming_msg errno and add errmsg in nts_errno.h
	 */

	int retval;

	//1. get `nt_sock_context` from `HashMap nt_sock_map` via sockid. 
	nt_sock_context_t nt_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx)
		goto FAIL;

	// TODO:HotFix check whether the socket state is valid or not
	// if(!nt_sock_ctx->socket || nt_sock_ctx->socket->state != CLOSED || nt_sock_ctx->socket->state != BOUND) {
	if(!nt_sock_ctx->socket || nt_sock_ctx->socket->state != CLOSED) {
		ERR("non-existing socket [sockid: %d], or the socket state is not `CLOSED`. ", sockid);
		goto FAIL;
	}

	//2. parse the sockaddr to get ip and port
	//4. pack the `NTM_MSG_CONNECT` ntm_msg and `ntm_shm_send()` the message into ntm
	struct sockaddr_in *sock = (struct sockaddr_in*) name;// TODO:delete change `&name` to `name`
	ntm_msg outgoing_msg;
	outgoing_msg.msg_id = nt_sock_ctx->ntm_msg_id;
	outgoing_msg.msg_type = NTM_MSG_CONNECT;
	outgoing_msg.sockid = sockid;
	outgoing_msg.port = ntohs(sock->sin_port);
	struct in_addr in = sock->sin_addr;
	inet_ntop(AF_INET, &in, outgoing_msg.address, sizeof(outgoing_msg.address));
	outgoing_msg.addrlen = strlen(outgoing_msg.address);
	DEBUG("nt_connect: %s:%d with sockid=%d, addrlen=%d", outgoing_msg.address, outgoing_msg.port, sockid, outgoing_msg.addrlen);


	// check whether the ip addr is valid or not
	// retval = ip_is_vaild(outgoing_msg.address);
	// if(retval){
	// 	ERR("the ip addr is not vaild. the current ip is %s, while the vaild ip is %s", outgoing_msg.address, NTS_CONFIG.nt_host);
	// 	goto FAIL;
	// }

	retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &outgoing_msg);
	if(retval){
		ERR("ntm_shm_send NTM_MSG_CONNECT message failed");
		goto FAIL;
	}

	//5. poll or wait for the response message [NTS_MSG_DISPATCHED msg] from ntm
	nts_msg incoming_msg;
	retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	while(retval){
		sched_yield();
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	}

	DEBUG("incoming_msg: msg_id=%d, retval=%d, msg_type=%d", incoming_msg.msg_id, incoming_msg.retval, incoming_msg.msg_type);
	if(incoming_msg.msg_id != outgoing_msg.msg_id){
		ERR("invalid message id for CONNECT response");
		goto FAIL;
	}
	if(incoming_msg.retval == -1){
		ERR("SOCKET CONNECT failed");
		goto FAIL;
	}
	if(incoming_msg.msg_type != NTS_MSG_DISPATCHED){
		ERR("invalid message type for CONNECT response. the current msg type is %d, expect msg type is %d", incoming_msg.msg_type, NTS_MSG_DISPATCHED);
		goto FAIL;
	}

	// update the client socket nt_port according to the returned incoming_msg.port
	nt_sock_ctx->socket->saddr.sin_port = htons(incoming_msg.port);

	DEBUG("incoming_msg: msg_id=%d, retval=%d, msg_type=%d, nt_port=%d", incoming_msg.msg_id, incoming_msg.retval, incoming_msg.msg_type, incoming_msg.port);
	DEBUG("socket[%d] receive NTS_MSG_DISPATCHED message, wait for NTS_MSG_ESTABLISH message.", sockid);

	//6. poll or wait for the response message [NTS_MSG_ESTABLISH msg] from ntm
	retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	while(retval){
		sched_yield();
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
	}

	// 7. parse the response nts_msg with nt_socket_id, 
	// 	if success: 
	// 		init/create ntp_send_ctx/ntp_recv_ctx according to 
	// 		[local_src_port]-[remote_dst_port]-[r/s] in corresponding nt_sock_context_t.
	//  	change socket state [ESTABLISHED] and socket type.
	if(incoming_msg.retval == -1){
		ERR("SOCKET CONNECT failed");
		goto FAIL;
	}
	if(incoming_msg.msg_type != NTS_MSG_ESTABLISH){
		ERR("invalid message type for CONNECT response. the current msg type is %d, expect msg type is %d", incoming_msg.msg_type, NTS_MSG_ESTABLISH);
		goto FAIL;
	}

	// init/create ntp_send_ctx/ntp_recv_ctx according to 
	// [local_src_port]-[remote_dst_port]-[r/s] in corresponding nt_sock_context_t.
	
	// get the shm name for ntp_send_ctx/ntp_recv_ctx: 
	// `[local client src_port]-[remote listen_dst_port]-r` for ntp recv shm name
	// `[local client src_port]-[remote listen_dst_port]-s` for ntp send shm name
	char recv_shmaddr[SHM_NAME_LEN], send_shmaddr[SHM_NAME_LEN];
	int client_src_port, listener_dst_port;
	listener_dst_port = parse_sockaddr_port((struct sockaddr_in *)name);
	client_src_port = parse_sockaddr_port(&nt_sock_ctx->socket->saddr);	
	sprintf(recv_shmaddr, "%d-%d-r", client_src_port, listener_dst_port);
	sprintf(send_shmaddr, "%d-%d-s", client_src_port, listener_dst_port);
	DEBUG("ntp send shm addr='%s', ntp recv shm addr='%s'", send_shmaddr, recv_shmaddr);

	// TODO:init/create ntp_send_ctx/ntp_recv_ctx
	nt_sock_ctx->ntp_recv_ctx = ntp_shm();
	nt_sock_ctx->ntp_send_ctx = ntp_shm();
	ntp_shm_connect(nt_sock_ctx->ntp_recv_ctx, recv_shmaddr, strlen(recv_shmaddr));
	ntp_shm_connect(nt_sock_ctx->ntp_send_ctx, send_shmaddr, strlen(send_shmaddr));
	
	// push the `nt_sock_context` into HashMap `nt_sock_map` in nts_context
	Put(nts_ctx->nt_sock_map, &sockid, nt_sock_ctx);

	// change socket state [ESTABLISHED] and socket type.
	nt_sock_ctx->socket->state = ESTABLISHED;
	DEBUG("nts_connect success");
	return 0;

	FAIL:
	return -1;
}

int nts_close(int sockid) {
	DEBUG("nts_close start running...");
	assert(nts_ctx);
	assert(sockid >= 0);

	/**
	 * 1. get/pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	 * 2. check whether the socket state is `CLOSED`
	 * 3. if socket state is `CLOSED`, then return 0
	 * 4. else if socket state is not `CLOSED`, check whether the socket state is `ESTABLSHED` or not
	 * 5. if not `ESTABLISHED`, directly destroy nt_socket related resources according to socktype:
	 * 		if socktype is `NT_SOCK_LISTENER`, destroy backlog_ctx, nts_shm_ctx, socket;
	 * 		else, destroy ntp_send_ctx, ntp_recv_ctx, nts_shm_ctx, socket
	 * 6. else if `ESTABLISHED`, set socket state to `WAIT_FIN`, send `NTP_NTS_MSG_FIN` ntp_msg to ntp
	 * 7. poll or wait the nts_shm queue to receive the `NTS_MSG_CLOSE` nts_msg from ntm
	 * 8. then set the socket state as `CLOSED` and destroy local ntb socket resources
	 */

	// 1. get/pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	HashMapIterator iter = createHashMapIterator(nts_ctx->nt_sock_map);
	while(hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		nt_sock_context_t tmp_sock_ctx;
		tmp_sock_ctx = (nt_sock_context_t) iter->entry->value;
		printf("{ key[sockid]=%d, sock_ctx->sockid=%d, hashcode=%d } \n", *(int *) iter->entry->key, iter->hashCode);

	}
	freeHashMapIterator(&iter);
	
	nt_sock_context_t nt_sock_ctx;
	nt_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx) {
		ERR("Non-existing sockid [sockid=%d]. ", sockid);
		goto FAIL;
	}
	// remove corresponding nt_sock_context_t from nts_ctx->nt_sock_map
	Remove(nts_ctx->nt_sock_map, &sockid);
	
	
	int retval;
	if(nt_sock_ctx->socket->socktype != NT_SOCK_LISTENER && nt_sock_ctx->socket->state != ESTABLISHED) {
		DEBUG("free UNESTABLISHED client nt_socket");
		nt_sock_ctx->socket->state = CLOSED;
		if (nt_sock_ctx->nts_shm_ctx) {
			nts_shm_close(nt_sock_ctx->nts_shm_ctx);
			nts_shm_destroy(nt_sock_ctx->nts_shm_ctx);
		}

		if (nt_sock_ctx->socket) {
			free(nt_sock_ctx->socket);
		}

		free(nt_sock_ctx);

		DEBUG("nts_close success [sockfd=%d]", sockid);
		return 0;
	}
	
	if (nt_sock_ctx->socket->socktype == NT_SOCK_LISTENER && nt_sock_ctx->socket->state == LISTENING) {
		DEBUG("free LISTENING server nt_socket");
		DEBUG("directly destroy nt_socket related resources.");
		nt_sock_ctx->socket->state = WAIT_FIN;

		//TODO: libnts send `NTM_MSG_CLOSE` to ntm, ntm destroy resources, then return ACK
		ntm_msg outgoing_msg;
		outgoing_msg.msg_id = nt_sock_ctx->ntm_msg_id;
		outgoing_msg.msg_type = NTM_MSG_CLOSE;
		outgoing_msg.sockid = sockid;
		retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &outgoing_msg);
		if (retval) {
			ERR("ntm_shm_send NTM_MSG_CLOSE with sockid[%d] failed.", sockid);
			goto FAIL;
		}

		// if listener nt_socket, first destroy backlog context
		if (nt_sock_ctx->socket->socktype == NT_SOCK_LISTENER) {
			DEBUG("destroy listener socket resources.");
			if(nt_sock_ctx->backlog_ctx) {
				backlog_nts_close(nt_sock_ctx->backlog_ctx);
				backlog_destroy(nt_sock_ctx->backlog_ctx);
			}

		}

		// poll or wait for the response message from ntm
		nts_msg incoming_msg;
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
		while(retval) {
			sched_yield();
			retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &incoming_msg);
		}

		// parse the response nts_msg with nt_socket_id
		if (incoming_msg.msg_id != outgoing_msg.msg_id) {
			ERR("invalid message id for NTM_MSG_CLOSE response");
			goto FAIL;
		}
		if (incoming_msg.msg_type != NTS_MSG_CLOSE) {
			ERR("invalid message type for NTM_MSG_CLOSE response");
			goto FAIL;
		}
		if (incoming_msg.retval == -1) {
			ERR("NTM_MSG_CLOSE failed");
			goto FAIL;
		}
		nt_sock_ctx->socket->state = CLOSED;

		if (nt_sock_ctx->socket->socktype != NT_SOCK_LISTENER) {
			
			if(nt_sock_ctx->ntp_send_ctx) {
				ntp_shm_close(nt_sock_ctx->ntp_send_ctx);
				ntp_shm_destroy(nt_sock_ctx->ntp_send_ctx);
			}
			if(nt_sock_ctx->ntp_recv_ctx) {
				ntp_shm_close(nt_sock_ctx->ntp_recv_ctx);
				ntp_shm_destroy(nt_sock_ctx->ntp_recv_ctx);
			}

		}

		if (nt_sock_ctx->nts_shm_ctx) {
			nts_shm_close(nt_sock_ctx->nts_shm_ctx);
			nts_shm_destroy(nt_sock_ctx->nts_shm_ctx);
		}

		if (nt_sock_ctx->socket) {
			free(nt_sock_ctx->socket);
		}

		free(nt_sock_ctx);
		
		DEBUG("nts_close listening server nt_socket success [sockfd=%d]", sockid);
		return 0;
	}


	// else if `ESTABLISHED`, directly destroy nt_socket related resources.
	DEBUG("actively disconnect/close active or ESTABLISHED client nt_socket [sockfd=%d]", sockid);
	nt_sock_ctx->socket->state = WAIT_FIN;



	/* ready to ntp shm send ntp_msg */
	shm_mempool_node *mp_node;
    mp_node = shm_mp_malloc(nt_sock_ctx->ntp_send_ctx->mp_handler, sizeof(ntp_msg));
    if (mp_node == NULL) {
        // perror("shm_mp_malloc failed. \n");
        return -1;
    }

	ntp_msg * ntp_outgoing_msg;
	ntp_outgoing_msg = (ntp_msg *) shm_offset_mem(nt_sock_ctx->ntp_send_ctx->mp_handler, mp_node->node_idx);
    if (ntp_outgoing_msg == NULL) {
        perror("shm_offset_mem failed \n");
        return -1;
    }


	ntp_outgoing_msg->msg_type = NTP_NTS_MSG_FIN;
	ntp_outgoing_msg->msg_len = 0;
	retval = ntp_shm_send(nt_sock_ctx->ntp_send_ctx, ntp_outgoing_msg);
	if(retval == -1) {
		ERR("ntp_shm_send NTP_NTS_MSG_FIN failed");
		goto FAIL;
	}
	// while (retval) {
	// 	sched_yield();
	// 	retval = ntp_shm_send(nt_sock_ctx->ntp_send_ctx, &ntp_outgoing_msg);
	// } 

	DEBUG("wait for FIN_ACK msg from remote monitor");
	nts_msg nts_incoming_msg;
	retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &nts_incoming_msg);
	while(retval) {
		sched_yield();
		retval = nts_shm_recv(nt_sock_ctx->nts_shm_ctx, &nts_incoming_msg);
	}

	// parse the response nts_msg with nt_socket_id
	if(nts_incoming_msg.retval == -1){
		ERR("SOCKET BIND failed");
		goto FAIL;
	}
	if(nts_incoming_msg.msg_type != NTS_MSG_CLOSE) {
		ERR("nts_close failed, the msg_type is not NTS_MSG_CLOSE");
		goto FAIL;
	}
	nt_sock_ctx->socket->state = CLOSED;

	// destroy socket related resources
	// i.e., ntp_send_ctx, ntp_recv_ctx, nts_shm_ctx, socket
	if(nt_sock_ctx->ntp_send_ctx) {
		ntp_shm_close(nt_sock_ctx->ntp_send_ctx);
		ntp_shm_destroy(nt_sock_ctx->ntp_send_ctx);
	}
	if(nt_sock_ctx->ntp_recv_ctx) {
		ntp_shm_close(nt_sock_ctx->ntp_recv_ctx);
		ntp_shm_destroy(nt_sock_ctx->ntp_recv_ctx);
	}

	if (nt_sock_ctx->nts_shm_ctx) {
		nts_shm_close(nt_sock_ctx->nts_shm_ctx);
		nts_shm_destroy(nt_sock_ctx->nts_shm_ctx);
	}

	if(nt_sock_ctx->socket) {
		free(nt_sock_ctx->socket);
		nt_sock_ctx->socket = NULL;
	}
	

	DEBUG("nts_close success [sockfd=%d]", sockid);
	return 0;

	FAIL: 

	return -1;
}

int nts_shutdown(int sockid, int how) {
	DEBUG("nts_shutdown start...");
	assert(nts_ctx);

	DEBUG("nts_shutdown success");
	return 0;
}



int nts_getpeername(int sockid, struct sockaddr *name,
    socklen_t *namelen) {

	return 0;
}

int nts_getsockname(int sockid, struct sockaddr *name,
    socklen_t *namelen) {

	return 0;
}



ssize_t nts_read(int sockid, void *buf, size_t nbytes) {
	DEBUG("nts_read start...");
	assert(nts_ctx);
	assert(sockid >= 0);

	if (!buf || nbytes <= 0)
	{
		ERR("the specified recv buf is NULL or the nbytes is <= 0. ");
		// set errno
		goto FAIL;
	}

	/**
	 * 1. get/pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	 * 2. check whether the socket state is `ESTABLISHED`, 
	 * 		if socket state is not `ESTABLISHED`, then set errno & return -1.
	 * 3. else, then check whether msg_type is `NTP_NTS_MSG_FIN`
	 * 4. if is `NTP_NTS_MSG_FIN`, then start to passively close nt_socket
	 * 5. else , pop/read/copy the data from ntp_recv_buf into params `buf`, 
	 * 		then return the read bytes
	 */

	// 1. get/pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	HashMapIterator iter = createHashMapIterator(nts_ctx->nt_sock_map);
	while(hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		nt_sock_context_t tmp_sock_ctx = (nt_sock_context_t)iter->entry->value;
		DEBUG("{ key = %d }", *(int *) iter->entry->key);
	}
	freeHashMapIterator(&iter);

	nt_sock_context_t nt_sock_ctx;
	nt_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx) {
		ERR("Non-existing sockid. ");
		goto FAIL;
	}


	// 2. check whether the socket state is `ESTABLISHED`, 
	//     if socket state is not `ESTABLISHED`, then set errno & return -1.
	if (nt_sock_ctx->socket->state != ESTABLISHED) {
		ERR("write() require `ESTABLISHED` socket first. ");
		//set errno 
		goto FAIL;
	}


	// 3. else, then check whether msg_type is `NTP_NTS_MSG_FIN`
	int retval;
	ntp_msg * incoming_data;
	DEBUG("[out] ntp_shm_front");
	// TODO: ntp_shm_recv is replaced with ntp_shm_front() method
	incoming_data = ntp_shm_front(nt_sock_ctx->ntp_recv_ctx);
	while (incoming_data == NULL)
	{
		sched_yield();
		incoming_data = ntp_shm_front(nt_sock_ctx->ntp_recv_ctx);
	}
	DEBUG("[out] ntp_shm_front end");


	// 4. if is `NTP_NTS_MSG_FIN`, then set the socket state to `WAIT_FIN` 
	// 	    and start to passively close nt_socket
	// two solution to destroy client socket-related resources in libnts and ntm
	// solution 1: nts send `NTM_MSG_FIN` message into ntm, then: 
	// 			set socket state to `CLOSED`, destroy local resources, 
	//			nts_shm_ntm_close()/nts_shm_close()[??] to close nts_shm.
	// solution 2: nts send `NTM_MSG_FIN` message into ntm, then:
	//			set socket state to `WAIT_FIN`, wait for the response ACK from ntm,
	//			set socket state to `CLOSED`, destroy local resources, 
	//			nts_shm_close() to unlink nts_shm.
	// To the end, we determine to use solution 1.
	if (incoming_data->msg_type == NTP_NTS_MSG_FIN) {
		DEBUG("Detect NTP_NTS_MSG_FIN msg to close ntb connection");
		handle_ntp_fin_msg(nt_sock_ctx, sockid);
		// set errno
		INFO("handle ntp fin msg success.");
		goto FAIL;
	}


	// 5. else , pop/read/copy the data from ntp_recv_buf into params `buf`, 
	//		then return the read bytes. The detailed workflow is the following:
	//  0. check whether there is data in ntp_buf[char[253]] of nt_sock_ctx, 
	//		if true, read payload in `ntp_buf`, follow the normal workflow.
	//	i. check whether the first ntp_msg msg_len < NTP_PAYLOAD_MAX_SIZE or not, 
	//	   if true, check whether the buf size >= msg_len or not, 
	//			if true, copy payload into buf, return msg_len;
	//			else, copy the buf-size part of payload into buf, cache the remaining
	//			payload into ntp_buf[char[253]] in nt_sock_ctx, return params `nbytes`;
	//	   else if msg_len == NTP_PAYLOAD_MAX_SIZE, then:
	//			check whether the buf size < msg_len or not:
	//			if true, then copy the buf-size part of payload into buf, cache the remaining
	//			 payload into ntp_buf[char[253]] in nt_sock_ctx, return params `nbytes`;
	//			else if buf size == msg_len, then copy payload into buf, return msg_len;
	//			else if buf size > msg_len, then copy payload into buf, 
	//			 update avaliable buf size and buf pointer, ntp_shm_recv()/ntp_shm_front() poll next message;
	//			judge whether msg_type is NTP_NTS_MSG_FIN or not, tack action.
	//			jump to `step 0`.


	// check whether there is data in ntp_buf[char[253]] of nt_sock_ctx
	char *payload;
	int payload_len;
	bool is_cached = false;
	if (nt_sock_ctx->ntp_buflen > 0) {
		payload = nt_sock_ctx->ntp_buf;
		payload_len = nt_sock_ctx->ntp_buflen;
		is_cached = true;
	} else {
		incoming_data = ntp_shm_recv(nt_sock_ctx->ntp_recv_ctx);
		if(incoming_data == NULL) {
			ERR("ntp_shm_recv failed");
			goto FAIL;
		}

		payload = incoming_data->msg;
		payload_len = incoming_data->msg_len;
	} 
	

	shm_mempool_node * tmp_node; 

	int bytes_left;	// indicate the payload length in each ntp_shm_recv
	int bytes_read, buf_avail_bytes;
	char *ptr;

	ptr = (char *) buf;	// point the recv buf
	buf_avail_bytes = nbytes;		// indicate the available bytes in recv buf
	bytes_left = payload_len;	// indicate the left payload length
	bytes_read = 0;				// indicate the bytes which have been copied into buf
	
	while (bytes_left > 0)
	{
		if (bytes_left < NTP_PAYLOAD_MAX_SIZE) {
			
			if(buf_avail_bytes >= bytes_left) {
				DEBUG("bytes_left < NTP_PAYLOAD_MAX_SIZE and buf_avail_bytes >= bytes_left with bytes_read=%d", bytes_read);
				memcpy(ptr + bytes_read, payload, bytes_left);
				bytes_read += bytes_left;

				if (is_cached) { // if the payload is copied from ntp_buf
					// memset(nt_sock_ctx->ntp_buf, 0, nt_sock_ctx->ntp_buflen);
					nt_sock_ctx->ntp_buflen = 0;
				}

				
				tmp_node = shm_mp_node_by_shmaddr(nt_sock_ctx->ntp_recv_ctx->mp_handler, (char *)incoming_data);
				DEBUG("tmp_node->node_idx=%d", tmp_node->node_idx);
				if(tmp_node) {
					shm_mp_free(nt_sock_ctx->ntp_recv_ctx->mp_handler, tmp_node);
				}

				DEBUG("bytes_read =%d, bytes_left = %d", bytes_read, bytes_left);
				
				break;

			} else {
				DEBUG("bytes_left < NTP_PAYLOAD_MAX_SIZE and buf_avail_bytes < bytes_left with bytes_read=%d", bytes_read);
				// memcpy(ptr + bytes_read, payload, bytes_left);
				memcpy(ptr + bytes_read, payload, buf_avail_bytes);
				payload += buf_avail_bytes;
				bytes_left -= buf_avail_bytes;
				bytes_read += buf_avail_bytes;
				memcpy(nt_sock_ctx->ntp_buf, payload, bytes_left);
				nt_sock_ctx->ntp_buflen = bytes_left;

				tmp_node = shm_mp_node_by_shmaddr(nt_sock_ctx->ntp_recv_ctx->mp_handler, (char *)incoming_data);
				DEBUG("tmp_node->node_idx=%d", tmp_node->node_idx);
				if(tmp_node) {
					shm_mp_free(nt_sock_ctx->ntp_recv_ctx->mp_handler, tmp_node);
				}

				break;

			}
		}
		else {	// msg_len == NTP_PAYLOAD_MAX_SIZE
			DEBUG("buf_avail_bytes < bytes_left with bytes_read=%d", bytes_read);
			if (buf_avail_bytes < bytes_left) {
				memcpy(ptr + bytes_read, payload, buf_avail_bytes);
				payload += buf_avail_bytes;
				bytes_left -= buf_avail_bytes;
				bytes_read += buf_avail_bytes;
				memcpy(nt_sock_ctx->ntp_buf, payload, bytes_left);
				nt_sock_ctx->ntp_buflen = bytes_left;

				tmp_node = shm_mp_node_by_shmaddr(nt_sock_ctx->ntp_recv_ctx->mp_handler, (char *)incoming_data);
				DEBUG("tmp_node->node_idx=%d", tmp_node->node_idx);
				if(tmp_node) {
					shm_mp_free(nt_sock_ctx->ntp_recv_ctx->mp_handler, tmp_node);
				}

				break;

			} 
			else if(buf_avail_bytes == bytes_left) {
				DEBUG("buf_avail_bytes == bytes_left with bytes_read=%d", bytes_read);
				memcpy(ptr + bytes_read, payload, bytes_left);
				bytes_read += bytes_left;

				if (is_cached) { // if the payload is copied from ntp_buf
					// memset(nt_sock_ctx->ntp_buf, 0, nt_sock_ctx->ntp_buflen);
					nt_sock_ctx->ntp_buflen = 0;
				}

				tmp_node = shm_mp_node_by_shmaddr(nt_sock_ctx->ntp_recv_ctx->mp_handler, (char *)incoming_data);
				DEBUG("tmp_node->node_idx=%d", tmp_node->node_idx);
				if(tmp_node) {
					shm_mp_free(nt_sock_ctx->ntp_recv_ctx->mp_handler, tmp_node);
				}
				
				break;

			} 
			else {	// avaliable buf size > msg_len or buf_avail_bytes > bytes_left

				//  copy payload into buf
				DEBUG("buf_avail_bytes > bytes_left with bytes_read=%d", bytes_read);
				memcpy(ptr + bytes_read, payload, bytes_left);

				// update avaliable buf size and buf pointer
				buf_avail_bytes -= bytes_left;
				// ptr += bytes_left;

				// update the read bytes and the left bytes to be read
				bytes_read += bytes_left;
				DEBUG("bytes_read =%d, bytes_left = %d", bytes_read, bytes_left);

				tmp_node = shm_mp_node_by_shmaddr(nt_sock_ctx->ntp_recv_ctx->mp_handler, (char *)incoming_data);
				DEBUG("tmp_node->node_idx=%d", tmp_node->node_idx);
				if(tmp_node) {
					shm_mp_free(nt_sock_ctx->ntp_recv_ctx->mp_handler, tmp_node);
				}

				

				/**
				 * ntp_shm_recv()/ntp_shm_front() poll next message
				 * judge whether msg_type is NTP_NTS_MSG_FIN or not
				 * if true, close(),
				 * else, jump to `step 0`
				 */
				DEBUG("[in] ntp_shm_front");
				incoming_data = ntp_shm_front(nt_sock_ctx->ntp_recv_ctx);
				while (incoming_data == NULL)
				{
					sched_yield();
					incoming_data = ntp_shm_front(nt_sock_ctx->ntp_recv_ctx);
				}
				DEBUG("[in] ntp_shm_front end");

				// judge whether msg_type is NTP_NTS_MSG_FIN or not
				if (incoming_data->msg_type == NTP_NTS_MSG_FIN) {
		
					handle_ntp_fin_msg(nt_sock_ctx, sockid);
					// set errno
					goto FAIL;
				} 

				if (nt_sock_ctx->ntp_buflen > 0) {
					payload = nt_sock_ctx->ntp_buf;
					payload_len = nt_sock_ctx->ntp_buflen;
					is_cached = true;
				} else {
					incoming_data = ntp_shm_recv(nt_sock_ctx->ntp_recv_ctx);
					if(incoming_data == NULL) {
						ERR("ntp_shm_recv failed");
						goto FAIL;
					}

					payload = incoming_data->msg;
					payload_len = incoming_data->msg_len;
				} 

				bytes_left = payload_len;

			}
		}


	}

	DEBUG("nts_read success with bytes_read=%d", bytes_read);
	return bytes_read;

	FAIL: 

	return -1;
}

ssize_t nts_readv(int fd, const struct iovec *iov, int iovcnt) {
	DEBUG("nts_readv start...");

	DEBUG("nts_readv success");
	return 0;
}



ssize_t nts_write(int sockid, const void *buf, size_t nbytes) {
	DEBUG("nts_write start with sockid = %d", sockid);
	if (!buf || nbytes <= 0)
	{
		ERR("the specified send buf is NULL or the nbytes is <= 0. ");
		return -1;
	}

	/**
	 * 1. get/pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	 * 2. check whether the socket state is `ESTABLISHED`, 
	 * 		if socket state is not `ESTABLISHED`, then set errno & return -1.
	 * 3. else, pack the data ntp_msg and push/write ntp_msg into ntp_send_buf
	 * 4. return the write bytes.
	 */

	// get/pop `nt_sock_context` from `HashMap nt_sock_map` using sockid.
	HashMapIterator iter = createHashMapIterator(nts_ctx->nt_sock_map);
	while(hasNextHashMapIterator(iter)) {
		iter = nextHashMapIterator(iter);
		nt_sock_context_t tmp_sock_ctx = (nt_sock_context_t)iter->entry->value;
		DEBUG("{ key = %d }", *(int *) iter->entry->key);
	}
	freeHashMapIterator(&iter);

	int retval;
	int write_bytes;
	nt_sock_context_t nt_sock_ctx;
	nt_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
	if(!nt_sock_ctx){
		ERR("Non-existing sockid");
		return -1;
		// goto FAIL;
	}
	// check whether the socket state is `ESTABLISHED`.
	if(nt_sock_ctx->socket->state != ESTABLISHED){
		ERR("write() require `ESTABLISHED` socket first.");
		goto FAIL;
	}

	// pack the data ntp_msg and push/write ntp_msg into ntp_send_buf
	write_bytes = 0;


	/* ready to ntp shm send ntp_msg */
	shm_mempool_node *mp_node;
    mp_node = shm_mp_malloc(nt_sock_ctx->ntp_send_ctx->mp_handler, sizeof(ntp_msg));
    if (mp_node == NULL) {
        // perror("shm_mp_malloc failed. \n");
        return -1;
    }

	ntp_msg *outgoing_msg;
	outgoing_msg = (ntp_msg *) shm_offset_mem(nt_sock_ctx->ntp_send_ctx->mp_handler, mp_node->node_idx);
    if (outgoing_msg == NULL) {
        perror("shm_offset_mem failed \n");
        return -1;
    }

	outgoing_msg->msg_type = NTP_NTS_MSG_DATA;
	outgoing_msg->msg_len = nbytes < NTP_PAYLOAD_MAX_SIZE ? nbytes : NTP_PAYLOAD_MAX_SIZE;
	memcpy(outgoing_msg->msg, buf, outgoing_msg->msg_len);
	DEBUG("ntp_shm_send NTP_NTS_MSG_DATA msg with msg_len=%d, outgoing_msg.msg='%s'",
		 outgoing_msg->msg_len, outgoing_msg->msg);
	retval = ntp_shm_send(nt_sock_ctx->ntp_send_ctx, outgoing_msg);
	if(retval == -1) {
		ERR("ntp_shm_send NTP_NTS_MSG_DATA failed");
		goto FAIL;
	}

	while(retval == 0){
		DEBUG("ntp_shm_send NTP_NTS_MSG_DATA msg success");

		// ntp_msg tmp_ntp_msg;
		// bool rs_front = ntp_shmring_front(nt_sock_ctx->ntp_send_ctx->ntsring_handle, &tmp_ntp_msg);
		// if (rs_front)
		// {
		// 	DEBUG("ntp_shmring_front with top ele msg_type=%d, msg_len=%d, msg='%s'", 
		// 		tmp_ntp_msg.msg_type, tmp_ntp_msg.msg_len, tmp_ntp_msg.msg);
		// } else {
		// 	DEBUG("no top element in ntp send shmring");
		// }
		

		write_bytes += outgoing_msg->msg_len;
		int next_msg_len;
		if (nbytes - write_bytes < NTP_PAYLOAD_MAX_SIZE)
			next_msg_len = nbytes - write_bytes;
		else
			next_msg_len = NTP_PAYLOAD_MAX_SIZE;

		if(next_msg_len <= 0){
			DEBUG("small msg pass");
			break;
		}


		/* ready to ntp shm send ntp_msg */
		shm_mempool_node * send_mp_node;
		send_mp_node = shm_mp_malloc(nt_sock_ctx->ntp_send_ctx->mp_handler, sizeof(ntp_msg));
		if (send_mp_node == NULL) {
			// perror("shm_mp_malloc failed. \n");
			return -1;
		}

		outgoing_msg = (ntp_msg *) shm_offset_mem(nt_sock_ctx->ntp_send_ctx->mp_handler, send_mp_node->node_idx);
		if (outgoing_msg == NULL) {
			perror("shm_offset_mem failed \n");
			return -1;
		}
		outgoing_msg->msg_type = NTP_NTS_MSG_DATA;
		outgoing_msg->msg_len = next_msg_len;

		memcpy(outgoing_msg->msg, buf+write_bytes, outgoing_msg->msg_len);
		retval = ntp_shm_send(nt_sock_ctx->ntp_send_ctx, outgoing_msg);
		if(retval == -1) {
			ERR("ntp_shm_send NTP_NTS_MSG_FIN failed");
			goto FAIL;
		}
	}
	
	if(retval == -1){
		ERR("ntp_shm_send NTP_NTS_MSG_DATA msg failed");
		if(nt_sock_ctx->socket->state == CLOSED){
			nt_sock_ctx->err_no = nts_ENETDOWN;
		} else if(nt_sock_ctx->socket->state == ESTABLISHED){
			/**
			 * TODO: judge whether the ntb send buffer is full or not.
			 */
			nt_sock_ctx->err_no = nts_ENOSPC;
		}
		goto FAIL;
	}

	DEBUG("nts_write success");
	return write_bytes;

	FAIL:

	ERR("nts_write failed!");
	return -1;
}

ssize_t nts_writev(int fd, const struct iovec *iov, int iovcnt) {
	DEBUG("nts_writev start...");

	DEBUG("nts_writev success");
	return 0;
}



ssize_t nts_send(int sockid, const void *buf, size_t len, int flags) {
	DEBUG("nts_send start...");

	DEBUG("nts_send success");
	return 0;
}

ssize_t nts_sendto(int sockid, const void *buf, size_t len, int flags,
    const struct sockaddr *to, socklen_t tolen) {
	DEBUG("nts_sendto start...");

	DEBUG("nts_sendto success");
	return 0;
}

ssize_t nts_sendmsg(int sockid, const struct msghdr *msg, int flags) {
	DEBUG("nts_sendmsg start...");

	DEBUG("nts_sendmsg success");
	return 0;
}



ssize_t nts_recv(int sockid, void *buf, size_t len, int flags) {
	DEBUG("nts_recv start...");

	DEBUG("nts_recv success");
	return 0;
}

ssize_t nts_recvfrom(int sockid, void *buf, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen) {
	DEBUG("nts_recvfrom start...");

	DEBUG("nts_recvfrom success");
	return 0;
}

ssize_t nts_recvmsg(int sockid, struct msghdr *msg, int flags) {
	DEBUG("nts_recvmsg start...");

	DEBUG("nts_recvmsg success");
	return 0;
}



int nts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout) {

	return 0;
}


int nts_poll(struct pollfd fds[], nfds_t nfds, int timeout) {

	return 0;
}


//int nts_kqueue(void) {
//
//	return 0;
//}
//
//int nts_kevent(int kq, const struct kevent *changelist, int nchanges,
//    struct kevent *eventlist, int nevents, const struct timespec *timeout) {
//
//	return 0;
//}
//
//int nts_kevent_do_each(int kq, const struct kevent *changelist, int nchanges,
//    void *eventlist, int nevents, const struct timespec *timeout,
//    void (*do_each)(void **, struct kevent *)) {
//
//	return 0;
//}



int nts_gettimeofday(struct timeval *tv, struct timezone *tz) {

	return 0;
}

int nts_dup(int oldfd) {

	return 0;
}

int nts_dup2(int oldfd, int newfd) {

	return 0;
}

/* POSIX-LIKE api end */


/* Tests if fd is used by NTSock */
int nts_fdisused(int fd) {

	return 0;
}

int nts_getmaxfd(void) {

	return 0;
}

