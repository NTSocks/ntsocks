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

#define MSG_FROM_NTS "Hello, Nt-Monitor! I am libnts app."



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

	int i;
	for (i = 0; i < 10; i++) {
		ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, MSG_FROM_NTS, sizeof(MSG_FROM_NTS));
	}

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

	nts_ntm_context_t ntm_ctx;
	nts_ntp_context_t ntp_ctx;
	int ret;

	/**
	 * read conf file and init libnts params
	 */
	nts_ctx = (nts_context_t) calloc(1, sizeof(struct nts_context));
	if(!nts_ctx) {
		perror("malloc");
		ERR("Failed to allocate nts_context.");
		return -1;
	}
	nts_ctx->ntm_ctx = (nts_ntm_context_t) calloc(1, sizeof(struct nts_ntm_context));
	if(!nts_ctx->ntm_ctx) {
		perror("malloc");
		ERR("Failed to allocate nts_ntm_context.");
		return -1;
	}
	ntm_ctx = nts_ctx->ntm_ctx;

	nts_ctx->ntp_ctx = (nts_ntp_context_t) calloc(1, sizeof(struct nts_ntp_context));
	if(!nts_ctx->ntp_ctx) {
		perror("malloc");
		ERR("Failed to allocate nts_ntp_context.");
		return -1;
	}
	ntp_ctx = nts_ctx->ntp_ctx;




	/**
	 * init the ntm shm ringbuffer to send the requests to ntb monitor
	 */
	nts_ctx->ntm_ctx->shm_send_ctx = ntm_shm();
	if(!nts_ctx->ntm_ctx->shm_send_ctx) {
		ERR("Failed to allocate ntm_shm_context.");
		return -1;
	}

	ret = ntm_shm_connect(nts_ctx->ntm_ctx->shm_send_ctx,
			NTM_SHMRING_NAME, sizeof(NTM_SHMRING_NAME));
	if(ret) {
		ERR("ntm_shm_accept failed.");
		return -1;
	}
	pthread_create(&ntm_ctx->shm_send_thr, NULL, ntm_send_thread, NULL);


	/**
	 * init the nts shm ringbuffer to receive the responses from ntb monitor
	 */



	/**
	 * init the ntp shm ringbuffer to send the data to ntb proxy
	 */


	/**
	 * init the nts shm ringbuffer to receive the response data from ntb proxy
	 */



	return 0;
}

void nts_destroy() {
	assert(nts_ctx);

	/**
	 * Destroy the ntm shmring resources
	 */
	ntm_shm_context_t shm_send_ctx;
	shm_send_ctx = nts_ctx->ntm_ctx->shm_send_ctx;
	if (shm_send_ctx && shm_send_ctx->shm_stat == NTM_SHM_READY) {
		ntm_shm_nts_close(shm_send_ctx);
		ntm_shm_destroy(shm_send_ctx);
	}

	nts_shm_context_t shm_recv_ctx;
	shm_recv_ctx = nts_ctx->ntm_ctx->shm_recv_ctx;
	if (shm_recv_ctx && shm_recv_ctx->shm_stat == NTS_SHM_READY) {
		nts_shm_close(shm_recv_ctx);
		nts_shm_destroy(shm_recv_ctx);
	}

	free(nts_ctx->ntp_ctx);
	free(nts_ctx->ntm_ctx);

	free(nts_ctx);
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

	/**
	 * test nts_init and nts_destroy
	 */
	DEBUG("entering nts_socket...");
	nts_init(NTS_CONFIG_FILE);
	getchar();
	nts_destroy();

	return 0;
}

int nts_setsockopt(int s, int level, int optname, const void *optval,
    socklen_t optlen) {

	return 0;
}

int nts_getsockopt(int s, int level, int optname, void *optval,
    socklen_t *optlen) {

	return 0;
}

int nts_listen(int s, int backlog) {
	return 0;
}

int nts_bind(int s, const struct sockaddr *addr, socklen_t addrlen){
	return 0;
}

int nts_accept(int s, const struct sockaddr *addr, socklen_t *addrlen) {

	return 0;
}

int nts_connect(int s, const struct sockaddr *name, socklen_t namelen) {

	return 0;
}

int nts_close(int fd) {

	return 0;
}

int nts_shutdown(int s, int how) {

	return 0;
}



int nts_getpeername(int s, struct sockaddr *name,
    socklen_t *namelen) {

	return 0;
}

int nts_getsockname(int s, struct sockaddr *name,
    socklen_t *namelen) {

	return 0;
}



ssize_t nts_read(int d, void *buf, size_t nbytes) {

	return 0;
}

ssize_t nts_readv(int fd, const struct iovec *iov, int iovcnt) {

	return 0;
}



ssize_t nts_write(int fd, const void *buf, size_t nbytes) {

	return 0;
}

ssize_t nts_writev(int fd, const struct iovec *iov, int iovcnt) {

	return 0;
}



ssize_t nts_send(int s, const void *buf, size_t len, int flags) {

	return 0;
}

ssize_t nts_sendto(int s, const void *buf, size_t len, int flags,
    const struct sockaddr *to, socklen_t tolen) {

	return 0;
}

ssize_t nts_sendmsg(int s, const struct msghdr *msg, int flags) {

	return 0;
}



ssize_t nts_recv(int s, void *buf, size_t len, int flags) {

	return 0;
}

ssize_t nts_recvfrom(int s, void *buf, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen) {

	return 0;
}

ssize_t nts_recvmsg(int s, struct msghdr *msg, int flags) {

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

