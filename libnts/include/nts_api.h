/*
 * <p>Title: nts_api.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTS_API_H_
#define NTS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "nts_event.h"
#include "nts_errno.h"


typedef int (*loop_func_t)(void *arg);

int nts_init(int argc, char * const argv[]);


/* POSIX-LIKE api begin */

int nts_fcntl(int fd, int cmd, ...);

int nts_sysctl(const int *name, u_int namelen, void *oldp, size_t *oldlenp,
    const void *newp, size_t newlen);

int nts_ioctl(int fd, unsigned long request, ...);

int nts_socket(int domain, int type, int protocol);

int nts_setsockopt(int s, int level, int optname, const void *optval,
    socklen_t optlen);

int nts_getsockopt(int s, int level, int optname, void *optval,
    socklen_t *optlen);

int nts_listen(int s, int backlog);
int nts_bind(int s, const struct linux_sockaddr *addr, socklen_t addrlen);
int nts_accept(int s, struct linux_sockaddr *addr, socklen_t *addrlen);
int nts_connect(int s, const struct linux_sockaddr *name, socklen_t namelen);
int nts_close(int fd);
int nts_shutdown(int s, int how);

int nts_getpeername(int s, struct linux_sockaddr *name,
    socklen_t *namelen);
int nts_getsockname(int s, struct linux_sockaddr *name,
    socklen_t *namelen);

ssize_t nts_read(int d, void *buf, size_t nbytes);
ssize_t nts_readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t nts_write(int fd, const void *buf, size_t nbytes);
ssize_t nts_writev(int fd, const struct iovec *iov, int iovcnt);

ssize_t nts_send(int s, const void *buf, size_t len, int flags);
ssize_t nts_sendto(int s, const void *buf, size_t len, int flags,
    const struct linux_sockaddr *to, socklen_t tolen);
ssize_t nts_sendmsg(int s, const struct msghdr *msg, int flags);

ssize_t nts_recv(int s, void *buf, size_t len, int flags);
ssize_t nts_recvfrom(int s, void *buf, size_t len, int flags,
    struct linux_sockaddr *from, socklen_t *fromlen);
ssize_t nts_recvmsg(int s, struct msghdr *msg, int flags);

int nts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout);

int nts_poll(struct pollfd fds[], nfds_t nfds, int timeout);

int nts_kqueue(void);
int nts_kevent(int kq, const struct kevent *changelist, int nchanges,
    struct kevent *eventlist, int nevents, const struct timespec *timeout);
int nts_kevent_do_each(int kq, const struct kevent *changelist, int nchanges,
    void *eventlist, int nevents, const struct timespec *timeout,
    void (*do_each)(void **, struct kevent *));

int nts_gettimeofday(struct timeval *tv, struct timezone *tz);

int nts_dup(int oldfd);
int nts_dup2(int oldfd, int newfd);

/* POSIX-LIKE api end */


/* Tests if fd is used by NTSock */
extern int nts_fdisused(int fd);

extern int nts_getmaxfd(void);





#ifdef __cplusplus
}
#endif


#endif /* NTS_API_H_ */
