#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define __USE_GNU
#include <sched.h>
#include <dlfcn.h>
#include <stdarg.h>

#include "libnts.h"
#include "nts_api.h"
#include "nts_epoll.h"
#include "nts_config.h"
#include "nt_log.h"

#ifdef ENABLE_DEBUG
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);
#else
DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);
#endif //ENABLE_DEBUG

#define __GNU_SOURCE

#ifndef likely
#define likely(x) __builtin_expect((x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

#define INIT_FUNCTION(func)                \
	real_##func = dlsym(RTLD_NEXT, #func); \
	assert(real_##func)

static int inited = 0;

static int (*real_close)(int);
static int (*real_socket)(int, int, int);
static int (*real_bind)(int, const struct sockaddr *, socklen_t);
static int (*real_connect)(int, const struct sockaddr *, socklen_t);
static int (*real_listen)(int, int);
static int (*real_setsockopt)(int, int, int, const void *, socklen_t);

static int (*real_accept)(int, struct sockaddr *, socklen_t *);
static int (*real_accept4)(int, struct sockaddr *, socklen_t *, int);
static int (*real_recv)(int, void *, size_t, int);
static int (*real_send)(int, const void *, size_t, int);

static ssize_t (*real_writev)(int, const struct iovec *, int);
static ssize_t (*real_write)(int, const void *, size_t);
static ssize_t (*real_read)(int, void *, size_t);
static ssize_t (*real_readv)(int, const struct iovec *, int);

static int (*real_ioctl)(int, int, void *);
static int (*real_fcntl)(int, int, ...);

static int (*real_select)(int, fd_set *,
						  fd_set *, fd_set *, struct timeval *);

static int (*real_epoll_create)(int);
static int (*real_epoll_create1)(int);
static int (*real_epoll_ctl)(int, int,
							 int, struct epoll_event *);
static int (*real_epoll_wait)(int,
							  struct epoll_event *, int, int);
static int (*real_epoll_pwait)(int,
							   struct epoll_event *, int, int, const __sigset_t *);

__attribute__((constructor)) void ntsocket_init(void)
{
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

	INIT_FUNCTION(epoll_create);
	INIT_FUNCTION(epoll_create1);
	INIT_FUNCTION(epoll_ctl);
	INIT_FUNCTION(epoll_wait);
	INIT_FUNCTION(epoll_pwait);

	DEBUG("ntsocket init pass!!!");
	nts_init(NTS_CONFIG_FILE);
	inited = 1;

	print_conf();
	return;
}

__attribute__((destructor)) void ntsocket_uninit(void)
{
	nts_destroy();
	DEBUG("ntsocket end.");

	return;
}

int socket(int domain, int type, int protocol)
{
	int fd;
	DEBUG("socket() with init_flag -- %d \n", nts_ctx->init_flag);

	if (unlikely(nts_ctx->init_flag == 0))
	{
		INIT_FUNCTION(socket);
		return real_socket(domain, type, protocol);
	}

	if ((AF_INET != domain) || (SOCK_STREAM != type && SOCK_DGRAM != type))
	{
		fd = real_socket(domain, type, protocol);
		return fd;
	}

	//TODO: add errno support
	fd = nts_socket(domain, type, protocol);
	// FD remmapping for handling the conflicts
	//	between nt_socket_id and Linux System FD
	if (fd >= 0)
	{
		nts_fd_remap(fd);
	}

	return fd;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(bind);
		return real_bind(sockfd, addr, addrlen);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_bind(sockfd, addr, addrlen);
	}
	else
	{
		return real_bind(sockfd, addr, addrlen);
	}
}

int connect(int sockfd,
			const struct sockaddr *addr, socklen_t addrlen)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(connect);
		return real_connect(sockfd, addr, addrlen);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_connect(sockfd, addr, addrlen);
	}
	else
	{
		return real_connect(sockfd, addr, addrlen);
	}
}

ssize_t send(int sockfd,
			 const void *buf, size_t len, int flags)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(send);
		return real_send(sockfd, buf, len, flags);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_send(sockfd, buf, len, flags);
	}
	else
	{
		return real_send(sockfd, buf, len, flags);
	}
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(recv);
		return real_recv(sockfd, buf, len, flags);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_recv(sockfd, buf, len, flags);
	}
	else
	{
		return real_recv(sockfd, buf, len, flags);
	}
}

int listen(int sockfd, int backlog)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(listen);
		return real_listen(sockfd, backlog);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_listen(sockfd, backlog);
	}
	else
	{
		return real_listen(sockfd, backlog);
	}
}

int setsockopt(int sockfd,
			   int level, int optname,
			   const void *optval, socklen_t optlen)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(setsockopt);
		return real_setsockopt(sockfd,
							   level, optname, optval, optlen);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_setsockopt(
			sockfd, level, optname, optval, optlen);
	}
	else
	{
		return real_setsockopt(sockfd,
							   level, optname, optval, optlen);
	}
}

int accept(int sockfd,
		   struct sockaddr *addr, socklen_t *addrlen)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(accept);
		return real_accept(sockfd, addr, addrlen);
	}

	if (nts_fdisused(sockfd))
	{
		int fd = nts_accept(sockfd, addr, addrlen);
		if (fd >= 0)
			nts_fd_remap(fd);

		return fd;
	}
	else
	{
		return real_accept(sockfd, addr, addrlen);
	}
}

int accept4(int sockfd,
			struct sockaddr *addr, socklen_t *addrlen, int flags)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(accept4);
		return real_accept4(sockfd, addr, addrlen, flags);
	}

	if (nts_fdisused(sockfd))
	{
		int fd = nts_accept4(sockfd, addr, addrlen, flags);
		if (fd >= 0)
		{
			nts_fd_remap(fd);
		}

		return fd;
	}
	else
	{
		return real_accept4(sockfd, addr, addrlen, flags);
	}
}

int close(int sockfd)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(close);
		return real_close(sockfd);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_close(sockfd);
	}
	else
	{
		return real_close(sockfd);
	}
}

ssize_t write(int sockfd, const void *buf, size_t count)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(write);
		return real_write(sockfd, buf, count);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_write(sockfd, buf, count);
	}
	else
	{
		return real_write(sockfd, buf, count);
	}
}

ssize_t writev(int sockfd,
			   const struct iovec *iov, int iovcnt)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(writev);
		return real_writev(sockfd, iov, iovcnt);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_writev(sockfd, iov, iovcnt);
	}
	else
	{
		return real_writev(sockfd, iov, iovcnt);
	}
}

ssize_t read(int sockfd, void *buf, size_t count)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(read);
		return real_read(sockfd, buf, count);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_read(sockfd, buf, count);
	}
	else
	{
		return real_read(sockfd, buf, count);
	}
}

ssize_t readv(int sockfd,
			  const struct iovec *iov, int iovcnt)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(readv);
		return real_readv(sockfd, iov, iovcnt);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_readv(sockfd, iov, iovcnt);
	}
	else
	{
		return real_readv(sockfd, iov, iovcnt);
	}
}

int ioctl(int sockfd, int request, void *p)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(ioctl);
		return real_ioctl(sockfd, request, p);
	}

	if (nts_fdisused(sockfd))
	{
		return nts_ioctl(sockfd, request, p);
	}
	else
	{
		return real_ioctl(sockfd, request, p);
	}
}

int fcntl(int sockfd, int cmd, ... /*arg*/)
{
	int rc;
	va_list args;
	va_start(args, cmd);

	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(fcntl);
		rc = real_fcntl(sockfd, cmd, args);
		va_end(args);
		return rc;
	}

	rc = nts_fcntl(sockfd, cmd, args);
	va_end(args);
	return rc;
}

int select(int nfds,
		   fd_set *readfds,
		   fd_set *writefds,
		   fd_set *exceptfds,
		   struct timeval *timeout)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(select);
		return real_select(nfds,
						   readfds, writefds, exceptfds, timeout);
	}

	if (nts_fdisused(nfds))
	{
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		return nts_select(nfds,
						  readfds, writefds, exceptfds, timeout);
	}
	else
	{
		return real_select(nfds,
						   readfds, writefds, exceptfds, timeout);
	}
}

int epoll_create(int size)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(epoll_create);
		return real_epoll_create(size);
	}

	return nts_epoll_create(size);
}

//TODO: support in the future
int epoll_create1(int flags)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(epoll_create1);
		return real_epoll_create1(flags);
	}

	return nts_epoll_create(flags);
}

int epoll_ctl(int epfd, int op,
			  int fd, struct epoll_event *event)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(epoll_ctl);
		return real_epoll_ctl(epfd, op, fd, event);
	}

	// map struct epoll_event to nts_epoll_event
	nts_epoll_event ep_event;
	ep_event.events = event->events;
	ep_event.data = event->data.fd;
	return nts_epoll_ctl(epfd, op, fd, &ep_event);
}

int epoll_wait(int epfd,
			   struct epoll_event *events,
			   int maxevents, int timeout)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(epoll_wait);
		return real_epoll_wait(
			epfd, events, maxevents, timeout);
	}

	// map struct epoll_event to nts_epoll_event
	nts_epoll_event *ep_events;
	ep_events = (nts_epoll_event *)
		calloc(maxevents, sizeof(nts_epoll_event));
	for (int i = 0; i < maxevents; i++)
	{
		ep_events[i].events = events[i].events;
		ep_events[i].data = events[i].data.fd;
	}

	int rc;
	rc = nts_epoll_wait(
		epfd, ep_events, maxevents, timeout);
	if (rc > 0)
	{
		for (int i = 0; i < rc; i++)
		{
			events[i].data.fd = ep_events[i].data;
			events[i].events = ep_events[i].events;
		}
	}

	return rc;
}

int epoll_pwait(int epfd,
				struct epoll_event *events,
				int maxevents, int timeout, const __sigset_t *ss)
{
	if (unlikely(inited == 0))
	{
		INIT_FUNCTION(epoll_pwait);
		return real_epoll_pwait(epfd,
								events, maxevents, timeout, ss);
	}

	// map struct epoll_event to nts_epoll_event
	// map struct epoll_event to nts_epoll_event
	nts_epoll_event *ep_events;
	ep_events = (nts_epoll_event *)
		calloc(maxevents, sizeof(nts_epoll_event));
	for (int i = 0; i < maxevents; i++)
	{
		ep_events[i].events = events[i].events;
		ep_events[i].data = events[i].data.fd;
	}

	int rc;
	rc = nts_epoll_wait(
		epfd, ep_events, maxevents, timeout);
	if (rc > 0)
	{
		for (int i = 0; i < rc; i++)
		{
			events[i].data.fd = ep_events[i].data;
			events[i].events = ep_events[i].events;
		}
	}

	return rc;
}
