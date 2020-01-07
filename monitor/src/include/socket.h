/*
 * <p>Title: socket.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef SOCKET_H_
#define SOCKET_H_

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

//	TAILQ_ENTRY (nt_socket) free_smap_link;

} nt_socket;

typedef struct nt_socket * nt_socket_t;


nt_socket_t allocate_socket(int socktype, int need_lock);

void free_socket(int sockid, int need_lock);

nt_socket_t get_socket(int sockid);


struct nt_listener {
	int sockid;
	nt_socket_t socket;

	int backlog;

	pthread_mutex_t accept_lock;
	pthread_cond_t accept_cond;

	/* hash table entry link */

};

typedef struct nt_listener * nt_listener_t;



// typedef struct nt_port {
// 	int port;
// 	int port_status;

// } nt_port;

typedef u_int16_t nt_port;


/**
 * automatically allocate a idle `port`
 */
nt_port allocate_port(int need_lock);

/**
 * Check if the specified `port` is used.
 * return: false when used, true when idle
 */
bool is_idle_port(int port);

/**
 * free the used port into free port queue
 */
void free_port(int port, int need_lock);


#ifdef __cplusplus
};
#endif

#endif /* SOCKET_H_ */
