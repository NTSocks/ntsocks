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
#include <sys/queue.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/*----------------------------------------------------------------------------*/
	/**
	 * Definitions for the shm communication between nt-monitor and nts app.
	 */
	typedef uint32_t nt_sock_id;

	typedef enum socket_type
	{
		NT_SOCK_UNUSED = 1,
		NT_SOCK_STREAM = 2, // default, for connect-based socket
		NT_SOCK_PROXY = 3,
		NT_SOCK_LISTENER = 4, // for listener socket
		NT_SOCK_EPOLL = 5,	  // for epoll socket
		NT_SOCK_PIPE = 6	  // for accepted-based socket
	} socket_type;

	typedef enum socket_state
	{
		CLOSED = 1,
		BOUND = 2,
		LISTENING = 3,
		WAIT_CLIENT = 4,

		WAIT_DISPATCH = 5,
		WAIT_SERVER = 6,

		ESTABLISHED = 7,
		WAIT_ESTABLISHED = 8,

		WAIT_CLOSE = 9	// WAIT_CLOSE -> CLOSED

	} socket_state;

	struct nt_socket
	{
		nt_sock_id sockid;
		int socktype;
		uint32_t opts;
		socket_state state;

		struct sockaddr_in saddr;

		union
		{
			struct nt_listener *listener;
			struct _nts_epoll *ep;
		};

		uint32_t epoll;
		uint32_t events;
		uint64_t ep_data;

		TAILQ_ENTRY(nt_socket)
		free_ntsock_link;
	} __attribute__((packed)) nt_socket;

	typedef struct nt_socket *nt_socket_t;

	struct nt_sock_context
	{
		nt_socket_t ntsock;
		TAILQ_HEAD(, nt_socket)
		free_ntsock;
		pthread_mutex_t socket_lock;
	} __attribute__((packed));

	typedef struct nt_sock_context *nt_sock_context_t;

	void init_socket_context(
		nt_sock_context_t ntsock_ctx, int max_concurrency);

	nt_socket_t allocate_socket(
		nt_sock_context_t ntsock_ctx, int socktype, int need_lock);

	void free_socket(
		nt_sock_context_t ntsock_ctx, int sockid, int need_lock);

	nt_socket_t get_socket(
		nt_sock_context_t ntsock_ctx, int sockid, int max_concurrency);

	struct nt_listener
	{
		int sockid;
		nt_socket_t socket;

		int backlog;

		pthread_mutex_t accept_lock;
		pthread_cond_t accept_cond;

		/* hash table entry link */
	} __attribute__((packed));

	typedef struct nt_listener *nt_listener_t;

	/**
	 * structure for in-flight nt_socket_t in backlog.
	 * the carrier on nt_backlog of nt_socket_t
	 */
	struct nt_backlog_sock
	{
		nt_sock_id sockid;
		int socktype;
		uint32_t opts;
		socket_state state;
		in_port_t sin_port; // the nt_port in network order
		in_addr_t sin_addr; // the ip address in network order
		sa_family_t sin_family;
	} __attribute__((packed)) nt_backlog_sock;

	typedef struct nt_backlog_sock *nt_backlog_sock_t;

#ifdef __cplusplus
};
#endif

#endif /* NT_SOCKET_H_ */
