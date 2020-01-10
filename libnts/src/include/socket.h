/*
 * <p>Title: socket.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef NT_SOCKET_H_
#define NT_SOCKET_H_

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/**
 * Definitions for the shm communication between nt-monitor and nts app.
 */
typedef uint64_t nt_sock_id;

typedef enum socket_type {
	NT_SOCK_UNUSED = 1,
	NT_SOCK_STREAM,  // default
	NT_SOCK_PROXY,
	NT_SOCK_LISTENER,
	NT_SOCK_EPOLL,
	NT_SOCK_PIPE
} socket_type;

typedef enum socket_state {
	CLOSED = 1,
	BOUND,
	LISTENING,
	WAIT_CLIENT,

	WAIT_DISPATCH,
	WAIT_SERVER,

	ESTABLISHED

} socket_state;


struct nt_socket {
	nt_sock_id sockid;
	int socktype;
	uint32_t opts;
	socket_state state;

	struct sockaddr_in saddr;

	union {
		struct nt_listener *listener;
	};

} nt_socket;

typedef struct nt_socket * nt_socket_t;

struct nt_listener {
	nt_sock_id sockid;
	nt_socket_t socket;

	int backlog;

	pthread_mutex_t accept_lock;
	pthread_cond_t accept_cond;

	/* hash table entry link */

};

typedef struct nt_listener * nt_listener_t;

#ifdef __cplusplus
};
#endif

#endif /* NT_SOCKET_H_ */
