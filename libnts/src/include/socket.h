#ifndef NT_SOCKET_H_
#define NT_SOCKET_H_

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>

#include "nts_epoll_inner.h"

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
		NT_SOCK_UNUSED = 0,
		NT_SOCK_STREAM, // default, for connect-based socket
		NT_SOCK_PROXY,
		NT_SOCK_LISTENER, // for listener socket
		NT_SOCK_EPOLL,	  // for epoll socket
		NT_SOCK_PIPE	  // for accepted-based socket
	} socket_type;

	typedef enum socket_state
	{
		CLOSED = 1,
		BOUND,
		LISTENING,
		WAIT_CLIENT,

		WAIT_DISPATCH,
		WAIT_SERVER,

		ESTABLISHED,
		WAIT_FIN 	// when active socket actively invokes close(), 
					//	update state to `WAIT_FIN`

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

	} nt_socket;

	typedef struct nt_socket *nt_socket_t;

	struct nt_listener
	{
		nt_sock_id sockid;
		nt_socket_t socket;

		int backlog;

		pthread_mutex_t accept_lock;
		pthread_cond_t accept_cond;

		/* hash table entry link */
	};

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
	} nt_backlog_sock;

	typedef struct nt_backlog_sock *nt_backlog_sock_t;

#ifdef __cplusplus
};
#endif

#endif /* NT_SOCKET_H_ */
