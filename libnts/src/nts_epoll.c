/*
 * <p>Title: nts_epoll.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#include "nts_epoll_inner.h"
#include "socket.h"
#include "epoll_event_queue.h"

#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "nts.h"
#include "nts_config.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

char *event_str[] = {"NONE", "IN", "PRI", "OUT", "ERR", "HUP", "RDHUP"};

char *EventToString(uint32_t event) {
	
	switch (event) {
		case NTS_EPOLLNONE : {
			return event_str[0];
		}
		case NTS_EPOLLIN : {
			return event_str[1];
		}
		case NTS_EPOLLPRI : {
			return event_str[2];
		}
		case NTS_EPOLLOUT : {
			return event_str[3];
		}
		case NTS_EPOLLERR : {
			return event_str[4];
		}
		case NTS_EPOLLHUP : {
			return event_str[5];
		}
		case NTS_EPOLLRDHUP : {
			return event_str[6];
		}
		default :
			return NULL;
	}

	return NULL;
}

static inline int check_epoll(nts_context_t nts_ctx, int epid);


static inline int check_epoll(nts_context_t nts_ctx, int epid) {
    if (epid < 0 || epid >= NTS_MAX_CONCURRENCY) {
        errno = EBADF;
        return -1;
    }

    nt_epoll_context_t epoll_ctx_;
    epoll_ctx_ = (nt_epoll_context_t)Get(nts_ctx->nt_epoll_map, &epid);
    if(!epoll_ctx_) {
        errno = ENOENT;
        return -1;
    }

    if (epoll_ctx_->socket->socktype == NT_SOCK_UNUSED) {
        errno = EINVAL;
        return -1;
    }

    if (epoll_ctx_->socket->socktype != NT_SOCK_EPOLL) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}


int nts_close_epoll_socket(int epid) {
    assert(nts_ctx);
    DEBUG("entering nts_close_epoll_socket()...");

    if (epid < 0 || epid >= NTS_MAX_CONCURRENCY) {
        errno = EBADF;
        ERR("Invalid epid for nts_close_epoll_socket()");
        return -1;
    }

    nt_epoll_context_t epoll_ctx = (nt_epoll_context_t)Get(nts_ctx->nt_epoll_map, &epid);
    if (!epoll_ctx) {
        errno = EINVAL;
        ERR("Non-existing epoll_context with epid %d", epid);
        return -1;
    }

    /**
     * 1. destroy the shm-based event_queue in local libnts by disconnecting it
     * 2. instruct ntm to remove the epoll of all listen socket within the specific epoll_socket
     * 3. [backend] ntm instruct ntp to remove the epoll of all client socket within the specific epoll_socket
     * 4. [backend] ntp disconnects the shm-based event_queue
     * 5. [backend] destroy shm-based event_queue thoroughly through ntm
     * 6. de-allocate and free the epoll socket 
     * 7. destroy epoll_shm_ring by libnts
     * 8. destroy the related epoll_context in local libnts
     */
    
    //  1. destroy the shm-based event_queue in local libnts by disconnecting it
    nt_socket_t ep_socket = epoll_ctx->socket;
    nts_epoll_t ep = ep_socket->ep;
    assert(ep);
    ep_event_queue_free(ep->usr_queue_ctx, false);
    ep->usr_queue = NULL;
    ep->mutex_sem = NULL;
    ep->full_sem = NULL;
    ep->empty_sem = NULL;
    free(ep);
    ep = NULL;

    // 2. instruct ntm to remove the epoll of all listen socket within the specific epoll_socket
    ntm_msg req_msg;
    req_msg.msg_id = epoll_ctx->epoll_msg_id;
    req_msg.msg_type = NTM_MSG_EPOLL_CLOSE;
    req_msg.epid = epid;
    req_msg.sock_type = NT_SOCK_EPOLL;
    int retval;
    retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &req_msg);
    if (retval) {
        ERR("ntm_shm_send() NTM_MSG_EPOLL_CLOSE message failed");
        goto FAIL;
    }

    // poll and wait for the NTM_MSG_EPOLL_CLOSE response message from ntm
    epoll_msg resp_msg;
    retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
    while(retval) {
        sched_yield();
        retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
    }

    // parse the response epoll_msg
    if (resp_msg.id != req_msg.msg_id || resp_msg.retval == -1) {
        ERR("invalid message id or retval == -1 for NTM_MSG_EPOLL_CLOSE response");
        goto FAIL;
    }
    epoll_ctx->epoll_msg_id++;

    // 6. de-allocate and free the epoll socket
    Remove(nts_ctx->nt_epoll_map, &epid);
    if (epoll_ctx->socket) {
        free(epoll_ctx->socket);
        epoll_ctx->socket = NULL;
    }

    // 7. destroy epoll_shm_ring by libnts
    if (epoll_ctx->epoll_shm_ctx) {
        epoll_shm_master_close(epoll_ctx->epoll_shm_ctx);
        epoll_shm_destroy(epoll_ctx->epoll_shm_ctx);
    }

    // 8. destroy the related epoll_context in local libnts
    free(epoll_ctx);
    epoll_ctx = NULL;

    return 0;

    FAIL:
    if (epoll_ctx->socket && epoll_ctx->socket->ep) {
        ep = epoll_ctx->socket->ep;
        ep_event_queue_free(ep->usr_queue_ctx, false);
        ep->usr_queue = NULL;
        ep->mutex_sem = NULL;
        ep->full_sem = NULL;
        ep->empty_sem = NULL;
        free(ep);
        ep = NULL;
    }

    if (epoll_ctx->epoll_shm_ctx) {
        epoll_shm_master_close(epoll_ctx->epoll_shm_ctx);
        epoll_shm_destroy(epoll_ctx->epoll_shm_ctx);
    }

    if (epoll_ctx->epoll_sockets) {
        HashMapIterator iter;
        iter = createHashMapIterator(epoll_ctx->epoll_sockets);
        while(hasNextHashMapIterator(iter)) {
            iter = nextHashMapIterator(iter);
            nt_socket_t sock = (nt_socket_t) iter->entry->value;

            sock->epoll = NTS_EPOLLNONE;
            sock->ep_data = 0;
            sock->events = 0;
            Remove(epoll_ctx->epoll_sockets, &sock->sockid);
        }
        freeHashMapIterator(&iter);
        Clear(epoll_ctx->epoll_sockets);
        epoll_ctx->epoll_sockets = NULL;
    }

    Remove(nts_ctx->nt_epoll_map, &epid);
    if (epoll_ctx->socket) {
        free(epoll_ctx->socket);
        epoll_ctx->socket = NULL;
    }

    free(epoll_ctx);
    epoll_ctx = NULL;

    return -1;
} 


int nts_epoll_flush_events(uint32_t cur_ts) {

    return -1;
}


int nts_raise_pending_stream_events(nts_epoll *ep, nt_socket_t socket) { 

    return -1;
}

// not thread-safe
int nts_epoll_create(int size) { 
    DEBUG("entering nts_epoll_create()... with size %d", size);
    assert(nts_ctx);
    DEBUG("assert nts_ctx pass");
    if (size < 0) {
        errno = EINVAL;
        return -1;
    }

    /**
     * 1. generate epoll_shm-uuid shm name.
     * 2. init/create epoll-corresponding epoll_shm_ring
     *      to receive response message from ntm.
     * 3. pack the `NTM_MSG_EPOLL_CREATE` ntm_msg and 
     *      `ntm_shm_send()` the message into ntm
     * 4. poll and wait for the response message from ntm
     * 5. parse the response nts_msg with nt_socket_id,
     *      if success, create corresponding nt_socket, 
     *      nt_epoll_context, and setup the nt_epoll_context
     * 7. push `nt_epoll_context` into `HashMap nt_epoll_map`,
     *      then return epoll socket_id
     */
    DEBUG("start calloc nt_epoll_context_t");
    nt_epoll_context_t nt_epoll_ctx;
    nt_epoll_ctx = (nt_epoll_context_t) calloc(1, sizeof(struct nt_epoll_context));
    if (nt_epoll_ctx) {
        return -1;
    }
    nt_epoll_ctx->epoll_sockets = createHashMap(NULL, NULL);

    nt_epoll_ctx->epoll_msg_id = 1;

    // generate epoll_shm-uuid shm name
    int retval;
    retval = generate_epoll_shmname(nt_epoll_ctx->epoll_shmaddr);
    if (retval == -1) {
        ERR("generate epoll_shm-uuid shm name failed");
        free(nt_epoll_ctx);
        return -1;
    }
    nt_epoll_ctx->epoll_shmlen = strlen(nt_epoll_ctx->epoll_shmaddr);
    DEBUG("generate epoll_shm-uuid shm name `%s`", nt_epoll_ctx->epoll_shmaddr);

    // init/create epoll-corresponding epoll_shm_ring
    DEBUG("init/create epoll-corresponding epoll_shm_ring");
    nt_epoll_ctx->epoll_shm_ctx = epoll_shm();
    if(!nt_epoll_ctx->epoll_shm_ctx) {
        ERR("epoll_shm() failed");
        free(nt_epoll_ctx);
        return -1;
    }
    DEBUG("start epoll_shm_accept");
    retval = epoll_shm_accept(nt_epoll_ctx->epoll_shm_ctx, 
            nt_epoll_ctx->epoll_shmaddr, nt_epoll_ctx->epoll_shmlen);
    if(retval != 0) {
        ERR("epoll_shm_accept() failed");
        free(nt_epoll_ctx);
        return -1;
    }
    DEBUG("epoll_shmaddr=%s, epoll_shmlen=%d", 
            nt_epoll_ctx->epoll_shmaddr, nt_epoll_ctx->epoll_shmlen);

    // pack the `NTM_MSG_EPOLL_CREATE` ntm_msg and 
    //      `ntm_shm_send()` the message into ntm
    DEBUG("start ntm_shm_send NTM_MSG_EPOLL_CREATE");
    ntm_msg req_msg;
    req_msg.msg_id = nt_epoll_ctx->epoll_msg_id;
    req_msg.msg_type = NTM_MSG_EPOLL_CREATE;
    req_msg.nts_shm_addrlen = nt_epoll_ctx->epoll_shmlen;
    memcpy(req_msg.nts_shm_name, nt_epoll_ctx->epoll_shmaddr, nt_epoll_ctx->epoll_shmlen);
    req_msg.sock_type = NT_SOCK_EPOLL;
    req_msg.io_queue_size = size;
    retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &req_msg);
    if(retval) {
        ERR("ntm_shm_send() NTM_MSG_EPOLL_CREATE message failed");
        epoll_shm_master_close(nt_epoll_ctx->epoll_shm_ctx);
        epoll_shm_destroy(nt_epoll_ctx->epoll_shm_ctx);
        free(nt_epoll_ctx);
        return -1;
    }

    // poll and wait for the epoll_create response message from ntm
    DEBUG("poll and wait for the epoll_create response message from ntm");
    epoll_msg resp_msg;
    retval = epoll_shm_recv(nt_epoll_ctx->epoll_shm_ctx, &resp_msg);
    while(retval) {
        sched_yield();
        retval = epoll_shm_recv(nt_epoll_ctx->epoll_shm_ctx, &resp_msg);
    }

    // parse the response epoll_msg
    DEBUG("parse the response epoll_msg");
    if (req_msg.msg_id != resp_msg.id 
            || resp_msg.retval == -1 
            || resp_msg.shm_addrlen <= 0) {
        ERR("invalid message id or retval == -1 for NTM_MSG_EPOLL_CREATE response");
        epoll_shm_master_close(nt_epoll_ctx->epoll_shm_ctx);
        epoll_shm_destroy(nt_epoll_ctx->epoll_shm_ctx);
        free(nt_epoll_ctx);
        errno = ENFILE;
        return -1;
    }

    nt_socket_t ep_socket;
    ep_socket = (nt_socket_t) calloc(1, sizeof(struct nt_socket));
    if (!ep_socket) {
        errno = ENFILE;
        ERR("malloc nt_socket_t failed");
        epoll_shm_master_close(nt_epoll_ctx->epoll_shm_ctx);
        epoll_shm_destroy(nt_epoll_ctx->epoll_shm_ctx);
        free(nt_epoll_ctx);
        return -1;
    }
    ep_socket->sockid = resp_msg.sockid;
    ep_socket->socktype = NT_SOCK_EPOLL;
    ep_socket->state = CLOSED;
    nt_epoll_ctx->socket = ep_socket;
    nt_epoll_ctx->epoll_msg_id ++;
    
    // sync epoll I/O queue shm name
    DEBUG("sync epoll I/O queue shm name");
    nt_epoll_ctx->io_queue_shmlen = resp_msg.shm_addrlen;
    nt_epoll_ctx->io_queue_size = size;
    memcpy(nt_epoll_ctx->io_queue_shmaddr, 
            resp_msg.shm_name, nt_epoll_ctx->io_queue_shmlen);
    
    nts_epoll *ep = (nts_epoll*) calloc(1, sizeof(nts_epoll));
    if (!ep) {
        ERR("malloc nts_epoll failed");
        free(ep_socket);
        epoll_shm_master_close(nt_epoll_ctx->epoll_shm_ctx);
        epoll_shm_destroy(nt_epoll_ctx->epoll_shm_ctx);
        free(nt_epoll_ctx);
        return -1;
    }

    //TODO: create/init SHM-based epoll I/O queue
    // init the epoll ready I/O queue, which is SHM-based.
    DEBUG("create/init SHM-based epoll I/O queue");
    ep->usr_queue_ctx = ep_event_queue_init(
                    nt_epoll_ctx->io_queue_shmaddr, nt_epoll_ctx->io_queue_shmlen, size);
    if (!ep->usr_queue_ctx) {
        ERR("malloc nts_epoll failed");
        free(ep);
        free(ep_socket);
        epoll_shm_master_close(nt_epoll_ctx->epoll_shm_ctx);
        epoll_shm_destroy(nt_epoll_ctx->epoll_shm_ctx);
        free(nt_epoll_ctx);
        return -1;
    }
    ep->usr_queue = ep->usr_queue_ctx->shm_queue;
    ep->mutex_sem = ep->usr_queue_ctx->mutex_sem;
    ep->full_sem = ep->usr_queue_ctx->full_sem;
    ep->empty_sem = ep->usr_queue_ctx->empty_sem;

    // ep->usr_queue = nts_create_event_queue(size);
    // if (!ep->usr_queue) {
    //     ERR("nts_create_event_queue failed");
    //     free(ep);
    //     free(ep_socket);    // need to notify ntm to deallocate the nt_socket_t id
    //     epoll_shm_master_close(nt_epoll_ctx->epoll_shm_ctx);
    //     epoll_shm_destroy(nt_epoll_ctx->epoll_shm_ctx);
    //     free(nt_epoll_ctx);
    //     return -1;
    // }

    ep_socket->ep = ep;

    // init the corresponding epoll mutex_lock and condition_variable
    // in our design, init Semaphore for sync the epoll ready I/O queue (SHM-based)


    // Insert new `nt_epoll_context_t` into HashMap
    Put(nts_ctx->nt_epoll_map, &ep_socket->sockid, nt_epoll_ctx);
    DEBUG("nts_epoll_create success");

    return ep_socket->sockid;
}


int nts_epoll_ctl(int epid, int op, int sockid, nts_epoll_event *event) {
    assert(nts_ctx);

    if (epid < 0 || epid >= NTS_MAX_CONCURRENCY) {
        errno = EBADF;
        return -1;
    }

    if (sockid < 0 || sockid >= NTS_MAX_CONCURRENCY) {
        errno = EBADF;
        return -1;
    }

    nt_epoll_context_t epoll_ctx;
    epoll_ctx = (nt_epoll_context_t)Get(nts_ctx->nt_epoll_map, &epid);
    if(!epoll_ctx) {
        errno = ENOENT;
        return -1;
    }

    if (epoll_ctx->socket->socktype == NT_SOCK_UNUSED) {
        errno = EINVAL;
        return -1;
    }

    if (epoll_ctx->socket->socktype != NT_SOCK_EPOLL) {
        errno = EINVAL;
        return -1;
    }

    uint32_t events;
    nt_sock_context_t sock_ctx;
    sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &sockid);
    if (!sock_ctx) {   
        // when user process invokes close(sockid), 
        // corresponding sock_context is destroyed,
        // so when sock_ctx is not found, return -1;
        Remove(epoll_ctx->epoll_sockets, &sockid);
        errno = ENOENT;
        return -1;
    }
    nt_socket_t socket = sock_ctx->socket;
    if (op == NTS_EPOLL_CTL_ADD) {
        if (socket->epoll) {
            errno = EEXIST;
            return -1;
        }

        // update local socket epoll status
        events = event->events;
        events |= (NTS_EPOLLERR | NTS_EPOLLHUP);
        socket->ep_data = event->data;
        socket->epoll = events;

        // update the status of socket-corresponding ntb-conn 
        // located at remote ntp process. 
        // handle path: libnts --> ntm --> ntp (update epoll status) 
        //                  --> ntm --> libnts

        // libnts --> ntm request
        // pack the `NTM_MSG_EPOLL_CTL` ntm_msg and 
        //  `ntm_shm_send()` the message into ntm
        ntm_msg req_msg;
        req_msg.msg_id = epoll_ctx->epoll_msg_id;
        req_msg.msg_type = NTM_MSG_EPOLL_CTL;
        req_msg.sockid = sockid;
        req_msg.sock_type = socket->socktype;
        req_msg.epid = epid;
        req_msg.epoll_op = NTS_EPOLL_CTL_ADD;
        req_msg.events = events;
        req_msg.ep_data = event->data;
        int retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &req_msg);
        if (retval) {
            ERR("ntm_shm_send() NTM_MSG_EPOLL_CTL message failed");
            return -1;
        }

        // poll and wait for the epoll_ctl response message from ntm
        epoll_msg resp_msg;
        retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
        while(retval) {
            sched_yield();
            retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
        }

        // parse the response epoll_msg
        if (req_msg.msg_id != resp_msg.id 
            || resp_msg.retval == -1) {
            ERR("invalid message id or retval == -1 for NTM_MSG_EPOLL_CREATE response");
            errno = ENFILE;
            return -1;
        }
        epoll_ctx->epoll_msg_id ++;
        Put(epoll_ctx->epoll_sockets, &socket->sockid, socket);
     
    } 
    else if (op == NTS_EPOLL_CTL_MOD) 
    {
        if (!socket->epoll) {
            errno = ENOENT;
            return -1;
        }
        // update local socket epoll status
        events = event->events;
        events |= (NTS_EPOLLERR | NTS_EPOLLHUP);
        socket->ep_data = event->data;
        socket->epoll = events;

        // update the status of socket-corresponding ntb-conn 
        // located at remote ntp process. 
        // handle path: libnts --> ntm --> ntp (update epoll status) 
        //                  --> ntm --> libnts

        // libnts --> ntm request
        // pack the `NTM_MSG_EPOLL_CTL` ntm_msg and 
        //      `ntm_shm_send()` the message into ntm
        ntm_msg req_msg;
        req_msg.msg_id = epoll_ctx->epoll_msg_id;
        req_msg.msg_type = NTM_MSG_EPOLL_CTL;
        req_msg.sockid = sockid;
        req_msg.sock_type = socket->socktype;
        req_msg.epid = epid;
        req_msg.epoll_op = NTS_EPOLL_CTL_MOD;
        req_msg.events = events;
        req_msg.ep_data = event->data;
        int retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &req_msg);
        if (retval) {
            ERR("ntm_shm_send() NTM_MSG_EPOLL_CTL message failed");
            return -1;
        }

        // poll and wait for the epoll_ctl response message from ntm
        epoll_msg resp_msg;
        retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
        while(retval) {
            sched_yield();
            retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
        }

        // parse the response epoll_msg
        if (req_msg.msg_id != resp_msg.id 
            || resp_msg.retval == -1) {
            ERR("invalid message id or retval == -1 for NTM_MSG_EPOLL_CREATE response");
            errno = ENFILE;
            return -1;
        }
        epoll_ctx->epoll_msg_id ++;
    } 
    else if (op == NTS_EPOLL_CTL_DEL)
    {
        if (!socket->epoll) {
            errno = ENOENT;
            return -1;
        }
        // update local socket epoll status
        socket->epoll = NTS_EPOLLNONE;

        // update the status of socket-corresponding ntb-conn 
        // located at remote ntp process. 
        // handle path: libnts --> ntm --> ntp (update epoll status) 
        //                  --> ntm --> libnts

        // libnts --> ntm request
        // pack the `NTM_MSG_EPOLL_CTL` ntm_msg and 
        //      `ntm_shm_send()` the message into ntm
        ntm_msg req_msg;
        req_msg.msg_id = epoll_ctx->epoll_msg_id;
        req_msg.msg_type = NTM_MSG_EPOLL_CTL;
        req_msg.sockid = sockid;
        req_msg.sock_type = socket->socktype;
        req_msg.epid = epid;
        req_msg.epoll_op = NTS_EPOLL_CTL_DEL;
        int retval = ntm_shm_send(nts_ctx->ntm_ctx->shm_send_ctx, &req_msg);
        if (retval) {
            ERR("ntm_shm_send() NTM_MSG_EPOLL_CTL message failed");
            return -1;
        }

        // poll and wait for the epoll_ctl response message from ntm
        epoll_msg resp_msg;
        retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
        while(retval) {
            sched_yield();
            retval = epoll_shm_recv(epoll_ctx->epoll_shm_ctx, &resp_msg);
        }

        // parse the response epoll_msg
        if (req_msg.msg_id != resp_msg.id 
            || resp_msg.retval == -1) {
            ERR("invalid message id or retval == -1 for NTM_MSG_EPOLL_CREATE response");
            errno = ENFILE;
            return -1;
        }
        epoll_ctx->epoll_msg_id ++;
        Remove(epoll_ctx->epoll_sockets, &socket->sockid);
    }

    return 0;
}

int nts_epoll_wait(int epid, nts_epoll_event *events, int maxevents, int timeout) { 
    if (!nts_ctx) return -1;

    if (check_epoll(nts_ctx, epid) != 0) return -1;

    nt_epoll_context_t epoll_ctx;
    epoll_ctx = (nt_epoll_context_t)Get(nts_ctx->nt_epoll_map, &epid);
    
    nts_epoll *ep = epoll_ctx->socket->ep;
    if (!ep || !events || maxevents <= 0) {
        errno = EINVAL;
		return -1;
    }
    ep->stat.calls ++;

    // if (pthread_mutex_lock(&ep->epoll_lock)) {
    //     if (errno == EDEADLK) {
	// 		ERR("nts_epoll_wait: epoll_lock blocked");
	// 	}
	// 	assert(0);
    // }

    /**
     * Epoll wait loop: 
     * 1. Wait for epoll events:
     *   if timeout > 0, then wait with a timeout, 
     *       return 0 when timed out while jumping out of the waiting loop
     *       when epoll events notification is received; 
     *   else if timeout == -1, then wait forever until epoll events are triggered.
     * 2. If epoll events notification is received, copy the epoll events one by one
     *      from (SHM-based) ready I/O event queue to user-specified `events` array,
     *      finally return the total number of received epoll events.
     */

    int cnt = 0;
    do {
        nts_event_queue_t eq = ep->usr_queue;
        // nts_event_queue_t eq_shadow = ep->usr_shadow_queue;

        while (eq->num_events == 0 
                && timeout != 0)
        {
            ep->stat.waits ++;
            ep->waiting = 1;

            if (timeout > 0) {
                struct timespec deadline;
                clock_gettime(CLOCK_REALTIME, &deadline);

                // default timeout unit is ms
                if (timeout >= 1000) {
                    int sec = timeout / 1000;
                    deadline.tv_sec += sec;
                    timeout -= sec * 1000;
                }
                deadline.tv_nsec += timeout * 1000000;

                if (deadline.tv_nsec >= 1000000000) {
                    deadline.tv_sec ++;
                    deadline.tv_nsec -= 1000000000;
                }

                // considering only one consumer for each epoll, don't use mutex sem
                int ret = sem_timedwait(ep->full_sem, &deadline);
                if (ret && errno != ETIMEDOUT) {
                    ERR("sem_timedwait failed. ret: %d, error: %s\n", 
							ret, strerror(errno));
                    return -1;
                }
                timeout = 0;
            } else if (timeout < 0) {
                DEBUG("[%s:%s:%d]: pthread_cond_wait\n", __FILE__, __func__, __LINE__);
                int ret = sem_wait(ep->full_sem);
                if(ret) {
                    ERR("sem_wait failed. ret: %d, error: %s\n", 
							ret, strerror(errno));
                    return -1;
                }
            }

            ep->waiting = 0;
            // judge whether the nts_context exits
            if (nts_ctx->exit) {
                errno = EINTR;
				return -1;
            }
        }

        int i;
        uint8_t validity;
        int num_events = eq->num_events;
        for(i = 0; i < num_events && cnt <= maxevents; i++) {
            nt_sock_context_t event_sock_ctx = (nt_sock_context_t) Get(nts_ctx->nt_sock_map, &eq->events[eq->start].sockid);
            if (!event_sock_ctx || event_sock_ctx->socket->sockid == 0) {
                eq->start++;
                eq->num_events--;
                if (eq->start >= eq->size) eq->start = 0;
                // tell spooler that there is a element to read: V (empty_sem)
                int retval = sem_post(ep->empty_sem);
                if (retval == -1) {
                    ERR("[ep_event_queue_pop()] sem_post: empty_sem");
                    return -1;
                }
            }
            
            nt_socket_t event_socket = event_sock_ctx->socket;
            validity = 1;
            if (event_socket->socktype == NT_SOCK_UNUSED) 
                validity = 0;
            if (!(event_socket->epoll & eq->events[eq->start].ev.events))
                validity = 0;
            // if (!(event_socket->events & eq->events[eq->start].ev.events))
            //     validity = 0;

            if (validity) {
                events[cnt++] = eq->events[eq->start].ev;
                assert(eq->events[eq->start].sockid >= 0);

                DEBUG("Socket %d: Handled event. event: %s, start: %u, end: %u, num: %u", 
                            event_socket->sockid, 
                            EventToString(eq->events[eq->start].ev.events), 
                            eq->start, eq->end, eq->num_events);
                ep->stat.handled ++;
            } else {
                DEBUG("Socket %d: event %s invalidated.\n", 
						eq->events[eq->start].sockid, 
						EventToString(eq->events[eq->start].ev.events));
                ep->stat.invalidated ++;
            }
            event_socket->events &= (~eq->events[eq->start].ev.events);

            eq->start++;
            eq->num_events--;
            if (eq->start >= eq->size) eq->start = 0;
            // tell spooler that there is a element to read: V (empty_sem)
            int retval = sem_post(ep->empty_sem);
            if (retval == -1) {
                ERR("[ep_event_queue_pop()] sem_post: empty_sem");
                return -1;
            }
        }

    } while (cnt == 0 && timeout != 0);

    return cnt;
}