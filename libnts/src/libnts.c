/*
 * <p>Title: libnts.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Nov 29, 2019 
 * @version 1.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <linux/eventpoll.h>
#include <unistd.h>
#include <assert.h>

#define __USE_GNU
#include <sched.h>
#include <dlfcn.h>

#include "libnts.h"
#include "nts_api.h"
#include "nts_config.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define __GNU_SOURCE

#ifndef likely
#define likely(x) __builtin_expect((x),1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((x),0)
#endif

#define INIT_FUNCTION(func) \
	real_##func = dlsym(RTLD_NEXT, #func); \
	assert(real_##func)

static int inited = 1;

static int (*real_close)(int);
static int (*real_socket)(int, int, int);
static int (*real_bind)(int, const struct sockaddr*, socklen_t);
static int (*real_connect)(int, const struct sockaddr*, socklen_t);
static int (*real_listen)(int, int);
static int (*real_setsockopt)(int, int, int, const void *, socklen_t);

static int (*real_accept)(int, struct sockaddr *, socklen_t *);
//static int (*real_accept4)(int, struct sockaddr *, socklen_t *, int);
static int (*real_recv)(int, void*, size_t, int);
static int (*real_send)(int, const void *, size_t, int);

static ssize_t (*real_writev)(int, const struct iovec *, int);
static ssize_t (*real_write)(int, const void *, size_t);
static ssize_t (*real_read)(int, void *, size_t);
static ssize_t (*real_readv)(int, const struct iovec *, int);

static int (*real_ioctl)(int, int, void *);
static int (*real_fcntl)(int, int, void *);

static int (*real_select)(int, fd_set *, fd_set *, fd_set *, struct timeval *);

__attribute__((constructor))
void ntsocket_init(void) {
	INIT_FUNCTION(socket);
	INIT_FUNCTION(bind);
	INIT_FUNCTION(connect);
	INIT_FUNCTION(close);
	INIT_FUNCTION(listen);
	INIT_FUNCTION(setsockopt);
	INIT_FUNCTION(accept);
	INIT_FUNCTION(recv);
	INIT_FUNCTION(send);
	INIT_FUNCTION(writev);
	INIT_FUNCTION(write);
	INIT_FUNCTION(read);
	INIT_FUNCTION(readv);

	INIT_FUNCTION(ioctl);
	INIT_FUNCTION(fcntl);
	INIT_FUNCTION(select);

	inited = 1;
	DEBUG("ntsocket init pass!!! \n");
	DEBUG("before load conf\n");
	print_conf();
	load_conf(NTS_CONFIG_FILE);
	DEBUG("after load conf\n");
	print_conf();
	return;
}

__attribute__((destructor))
void ntsocket_uninit(void) {

	return;
}

int socket(int domain, int type, int protocol) {
	int ret;
	DEBUG("socket() with inited -- %d \n", inited);

	if (unlikely(inited == 0)) {
		INIT_FUNCTION(socket);
		return real_socket(domain, type, protocol);
	}

	if ((AF_INET != domain) || (SOCK_STREAM != type && SOCK_DGRAM != type)) {
		ret = real_socket(domain, type, protocol);
		return ret;
	}

	ret = nts_socket(domain, type, protocol);


	return 0;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(bind);
		return real_bind(sockfd, addr, addrlen);
	}

	if (nts_fdisused(sockfd)) {
		return nts_bind(sockfd, addr, addrlen);
	} else {
		return real_bind(sockfd, addr, addrlen);
	}
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(connect);
		return real_connect(sockfd, addr, addrlen);
	}

	if (nts_fdisused(sockfd)) {
		return nts_connect(sockfd, addr, addrlen);
	} else {
		return real_connect(sockfd, addr, addrlen);
	}
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(send);
		return real_send(sockfd, buf, len, flags);
	}

	if (nts_fdisused(sockfd)) {
		return nts_send(sockfd, buf, len, flags);
	} else {
		return real_send(sockfd, buf, len, flags);
	}
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(recv);
		return real_recv(sockfd, buf, len, flags);
	}

	if (nts_fdisused(sockfd)) {
		return nts_recv(sockfd, buf, len, flags);
	} else {
		return real_recv(sockfd, buf, len, flags);
	}
}

int listen(int sockfd, int backlog) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(listen);
		return real_listen(sockfd, backlog);
	}

	if (nts_fdisused(sockfd)) {
		return nts_listen(sockfd, backlog);
	} else {
		return real_listen(sockfd, backlog);
	}
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
		socklen_t optlen) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(setsockopt);
		return real_setsockopt(sockfd, level, optname, optval, optlen);
	}

	if (nts_fdisused(sockfd)) {
		return nts_setsockopt(sockfd, level, optname, optval, optlen);
	} else {
		return real_setsockopt(sockfd, level, optname, optval, optlen);
	}
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(accept);
		return real_accept(sockfd, addr, addrlen);
	}

	if (nts_fdisused(sockfd)) {
		return nts_accept(sockfd, addr, addrlen);
	} else {
		return real_accept(sockfd, addr, addrlen);
	}
}

//int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
//	if (unlikely(inited == 0)) {
//		INIT_FUNCTION(accept4);
//		return real_accept4(sockfd, addr, addrlen, flags);
//	}
//
//	if (nts_fdisused(sockfd)) {
//		return nts_accept4(sockfd, addr, addrlen, flags);
//	} else {
//		return real_accept4(sockfd, addr, addrlen, flags);
//	}
//}

int close(int sockfd) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(close);
		return real_close(sockfd);
	}

	if (nts_fdisused(sockfd)) {
		return nts_close(sockfd);
	} else {
		return real_close(sockfd);
	}
}

ssize_t write(int sockfd, const void *buf, size_t count) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(write);
		return real_write(sockfd, buf, count);
	}

	if (nts_fdisused(sockfd)) {
		return nts_write(sockfd, buf, count);
	} else {
		return real_write(sockfd, buf, count);
	}
}

ssize_t writev(int sockfd, const struct iovec *iov, int iovcnt) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(writev);
		return real_writev(sockfd, iov, iovcnt);
	}

	if (nts_fdisused(sockfd)) {
		return nts_writev(sockfd, iov, iovcnt);
	} else {
		return real_writev(sockfd, iov, iovcnt);
	}
}

ssize_t read(int sockfd, void *buf, size_t count) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(read);
		return real_read(sockfd, buf, count);
	}

	if (nts_fdisused(sockfd)) {
		return nts_read(sockfd, buf, count);
	} else {
		return real_read(sockfd, buf, count);
	}
}

ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(readv);
		return real_readv(sockfd, iov, iovcnt);
	}

	if (nts_fdisused(sockfd)) {
		return nts_readv(sockfd, iov, iovcnt);
	} else {
		return real_readv(sockfd, iov, iovcnt);
	}
}

int ioctl(int sockfd, int request, void *p) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(ioctl);
		return real_ioctl(sockfd, request, p);
	}

	if (nts_fdisused(sockfd)) {
		return nts_ioctl(sockfd, request, p);
	} else {
		return real_ioctl(sockfd, request, p);
	}
}

int fcntl(int sockfd, int cmd, void *p) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(fcntl);
		return real_fcntl(sockfd, cmd, p);
	}

	if (nts_fdisused(sockfd)) {
		return nts_fcntl(sockfd, cmd, p);
	} else {
		return real_fcntl(sockfd, cmd, p);
	}
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout) {
	if (unlikely(inited == 0)) {
		INIT_FUNCTION(select);
		return real_select(nfds, readfds, writefds, exceptfds, timeout);
	}

	if (nts_fdisused(nfds)) {
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		return nts_select(nfds, readfds, writefds, exceptfds, timeout);
	} else {
		return real_select(nfds, readfds, writefds, exceptfds, timeout);
	}
}

