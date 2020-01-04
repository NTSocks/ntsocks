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

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/**
 * Definitions for the shm communication between nt-monitor and nts app.
 */
typedef uint64_t nt_sock_id;

typedef enum socket_type {
	NT_SOCK_UNUSED,
	NT_SOCK_STREAM,  // default
	NT_SOCK_PROXY,
	NT_SOCK_LISTENER,
	NT_SOCK_EPOLL,
	NT_SOCK_PIPE
} socket_type;


struct nt_socket {
	nt_sock_id sockid;
	int socktype;
	uint32_t opts;

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


#ifdef __cplusplus
};
#endif

#endif /* SOCKET_H_ */
