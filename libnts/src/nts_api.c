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

#include "nts_api.h"
#include "nts_config.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);



static void * ntm_recv_thread(void *arg);

static void * ntm_send_thread(void *arg);

static void * ntp_recv_thread(void *arg);

static void * ntp_send_thread(void *arg);

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
	 * 2. init/create nts_socket-coresponding nts_shm_ring 
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
	ntm_msg outgoing_msg;
	outgoing_msg.msg_id = 1;
	outgoing_msg.msg_type = NTM_MSG_NEW_SOCK;
	outgoing_msg.nts_shm_addrlen = nt_sock_ctx->nts_shmlen;
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

	// push `nt_sock_context` into `HashMap nt_sock_map`, then return sockid
	Put(nts_ctx->nt_sock_map, &socket->sockid, nt_sock_ctx);
	DEBUG("nts_socket() pass");

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

	DEBUG("nts_setsockopt() pass");
	return 0;
}

int nts_getsockopt(int sockid, int level, int optname, void *optval,
    socklen_t *optlen) {
	DEBUG("nts_getsockopt() start...");
	assert(nts_ctx);

	DEBUG("nts_getsockopt() pass");
	return 0;
}

int nts_listen(int sockid, int backlog) {
	DEBUG("nts_listen() start...");
	assert(nts_ctx);

	DEBUG("nts_listen() pass");

	return 0;
}

int nts_bind(int sockid, const struct sockaddr *addr, socklen_t addrlen){
	DEBUG("nts_bind() start...");
	assert(nts_ctx);

	DEBUG("nts_bind() pass");
	return 0;
}

int nts_accept(int sockid, const struct sockaddr *addr, socklen_t *addrlen) {
	DEBUG("nts_accept start...");
	assert(nts_ctx);

	/**
	 * 1. get the coressponding nt_sock_context via nt_socket_id
	 */


	DEBUG("nts_accept pass");
	return 0;
}

int nts_connect(int sockid, const struct sockaddr *name, socklen_t namelen) {
	DEBUG("nts_connect start...");
	assert(nts_ctx);

	DEBUG("nts_connect pass");
	return 0;
}

int nts_close(int fd) {
	DEBUG("nts_close start...");
	assert(nts_ctx);

	DEBUG("nts_close pass");
	return 0;
}

int nts_shutdown(int sockid, int how) {
	DEBUG("nts_shutdown start...");
	assert(nts_ctx);

	DEBUG("nts_shutdown pass");
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



ssize_t nts_read(int d, void *buf, size_t nbytes) {
	DEBUG("nts_read start...");

	DEBUG("nts_read pass");
	return 0;
}

ssize_t nts_readv(int fd, const struct iovec *iov, int iovcnt) {
	DEBUG("nts_readv start...");

	DEBUG("nts_readv pass");
	return 0;
}



ssize_t nts_write(int fd, const void *buf, size_t nbytes) {
	DEBUG("nts_write start...");

	DEBUG("nts_write pass");
	return 0;
}

ssize_t nts_writev(int fd, const struct iovec *iov, int iovcnt) {
	DEBUG("nts_writev start...");

	DEBUG("nts_writev pass");
	return 0;
}



ssize_t nts_send(int sockid, const void *buf, size_t len, int flags) {
	DEBUG("nts_send start...");

	DEBUG("nts_send pass");
	return 0;
}

ssize_t nts_sendto(int sockid, const void *buf, size_t len, int flags,
    const struct sockaddr *to, socklen_t tolen) {
	DEBUG("nts_sendto start...");

	DEBUG("nts_sendto pass");
	return 0;
}

ssize_t nts_sendmsg(int sockid, const struct msghdr *msg, int flags) {
	DEBUG("nts_sendmsg start...");

	DEBUG("nts_sendmsg pass");
	return 0;
}



ssize_t nts_recv(int sockid, void *buf, size_t len, int flags) {
	DEBUG("nts_recv start...");

	DEBUG("nts_recv pass");
	return 0;
}

ssize_t nts_recvfrom(int sockid, void *buf, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen) {
	DEBUG("nts_recvfrom start...");

	DEBUG("nts_recvfrom pass");
	return 0;
}

ssize_t nts_recvmsg(int sockid, struct msghdr *msg, int flags) {
	DEBUG("nts_recvmsg start...");

	DEBUG("nts_recvmsg pass");
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

