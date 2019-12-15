/*
 * <p>Title: nts_api.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 9, 2019 
 * @version 1.0
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "nts_api.h"

int nts_init(int argc, char * const argv[]) {

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

#ifdef __cplusplus
}
#endif