/*
 * <p>Title: nts_epoll_inner.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date July 21, 2020
 * @version 1.0
 */

#ifndef __NTS_EPOLL_INNER__H__
#define __NTS_EPOLL_INNER__H__

#include "nts_epoll.h"
#include "epoll_event_queue.h"

typedef struct _nts_epoll_stat {
	uint64_t calls;
	uint64_t waits;
	uint64_t wakes;

	uint64_t issued;
	uint64_t registered;
	uint64_t invalidated;
	uint64_t handled;
} nts_epoll_stat;


typedef enum {
	USR_EVENT_QUEUE = 0,
	USR_SHADOW_EVENT_QUEUE = 1,
	NTS_EVENT_QUEUE = 2
} nts_event_queue_type;

/**
 * @brief  'nts_event_queue' should be SHM-based, and shared by libnts,
 *          ntm, and ntp.
 * @note   
 * @retval None
 */
// typedef struct _nts_event_queue {
// 	nts_epoll_event_int *events;
// 	int start;
// 	int end;
// 	int size;
// 	int num_events;
// } nts_event_queue;

typedef struct _nts_epoll {
	nts_event_queue_t usr_queue;
	epoll_event_queue_t usr_queue_ctx;

	uint8_t waiting;
	nts_epoll_stat stat;

	sem_t *mutex_sem;
    sem_t *full_sem;
    sem_t *empty_sem;

	// pthread_cond_t epoll_cond;
	// pthread_mutex_t epoll_lock;
} nts_epoll;
typedef struct _nts_epoll * nts_epoll_t;

// int nts_epoll_add_event(nts_epoll *ep, int queue_type, struct nt_socket *socket, uint32_t event);
int nts_close_epoll_socket(int epid);
int nts_epoll_flush_events(uint32_t cur_ts);

#endif  //!__NTS_EPOLL_INNER__H__