/*
 * <p>Title: nts_epoll.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTS_EPOLL_H_
#define NTS_EPOLL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/epoll.h>

int nts_epoll_create(int size);
int nts_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int nts_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);


#ifdef __cplusplus
}
#endif

#endif /* NTS_EPOLL_H_ */
