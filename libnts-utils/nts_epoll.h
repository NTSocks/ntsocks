#ifndef NTS_EPOLL_H_
#define NTS_EPOLL_H_

#include <sys/epoll.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum
	{
		NTS_EPOLLNONE = 0x0000,
		NTS_EPOLLIN = 0x0001,
		NTS_EPOLLPRI = 0x0002,
		NTS_EPOLLOUT = 0x0004,
		NTS_EPOLLRDNORM = 0x0040,
		NTS_EPOLLRDBAND = 0x0080,
		NTS_EPOLLWRNORM = 0x0100,
		NTS_EPOLLWRBAND = 0x0200,
		NTS_EPOLLMSG = 0x0400,
		NTS_EPOLLERR = 0x0008,
		NTS_EPOLLHUP = 0x0010,
		NTS_EPOLLRDHUP = 0x2000,
		NTS_EPOLLONESHOT = (1 << 30),
		NTS_EPOLLET = (1 << 31)

	} nts_epoll_type;

	typedef enum
	{
		NTS_EPOLL_CTL_ADD = 1,
		NTS_EPOLL_CTL_DEL = 2,
		NTS_EPOLL_CTL_MOD = 3,
	} nts_epoll_op;

	typedef union _nts_epoll_data
	{
		void *ptr;
		int sockid;
		uint32_t u32;
		uint64_t u64;
	} nts_epoll_data;

	typedef struct _nts_epoll_event
	{
		uint64_t data;
		uint32_t events;
	} __attribute__((packed)) nts_epoll_event;

	int nts_epoll_create(int size);
	int nts_epoll_ctl(int epid, int op,
					  int sockid, nts_epoll_event *event);
	int nts_epoll_wait(int epid,
					   nts_epoll_event *events, int maxevents, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* NTS_EPOLL_H_ */
