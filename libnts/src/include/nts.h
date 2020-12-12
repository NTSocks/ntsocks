/*
 * <p>Title: nts.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTS_H_
#define NTS_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/queue.h>

#include "nts_shm.h"
#include "ntm_shm.h"
#include "socket.h"
#include "hash_map.h"
#include "nt_backlog.h"
#include "ntp2nts_shm.h"
#include "epoll_shm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NTM_SHMRING_NAME "/ntm-shm-ring"
#define IP_ADDRLEN	16

struct nts_config {
	int tcp_timewait;
	int tcp_timeout;
	uint16_t mtu_size;	// equal to max data packet size
	uint16_t local_hostlen;
	char local_nt_host[IP_ADDRLEN];
	uint16_t max_payloadsize;
};

typedef struct nt_host_entry {
	char ipaddr[IP_ADDRLEN];
	uint16_t ipaddrlen;
	TAILQ_ENTRY(nt_host_entry) entries;
} nt_host_entry;

typedef struct nt_host_entry* nt_host_entry_t;
TAILQ_HEAD(, nt_host_entry) nt_host_head;


/*----------------------------------------------------------------------------*/
/**
 * Definitions for nt_socket-level context to manage:
 * 1. the control message communication between libnts and ntb monitor.
 * 2. the data message communication between libnts and ntb proxy.
 */

typedef struct nt_sock_context {
	nt_socket_t socket;

	/**
	 * the nts shm channel between nt_socket and ntm
	 * 
	 * 1. init or create first when invoking `socket()`
	 */
	char nts_shmaddr[SHM_NAME_LEN];
	int nts_shmlen;
	nts_shm_context_t nts_shm_ctx;
	int ntm_msg_id; 		// act as the global ntm_msg_id

	union
	{
		nt_backlog_context_t backlog_ctx;	// for listener socket
		nt_socket_t listener_socket;		// for client socket 
	};
	
	/**
	 * Data Plane:
	 * the ntp shm ring queue for send/receive message
	 * 
	 * 1. init or create first when invoking `connect()` or `accept()` finished
	 */
	ntp_shm_context_t ntp_send_ctx;			// send data message from libnts to ntp
	ntp_shm_context_t ntp_recv_ctx;			// receive data message from ntp to libnts
	int ntp_msg_id;							// act as the global ntp_msg_id
	int err_no;								// error number when read/write data message from/to ntp
	
	char *ntp_buf;		// cache the remaining un-read/recv payload
	int ntp_buflen; 						// the length of the remaining un-read/recv payload

}__attribute__((packed)) nt_sock_context;

typedef struct nt_sock_context* nt_sock_context_t;



/*----------------------------------------------------------------------------*/
/**
 * Definitions for the shm communication between libnts and ntb monitor.
 */

struct nts_ntm_context {

	/**
	 * the ntm shm ring queue between nts and ntm.
	 * 
	 * 1. created by ntb monitor first
	 * 2. used by libnts to send control messages into ntb monitor
	 * when invoking `socket()`, `connect()`, `bind()`, `listen()`, `accept()`, `close()`
	 * 
	 */
	ntm_shm_context_t shm_send_ctx;


};

typedef struct nts_ntm_context* nts_ntm_context_t;



/*----------------------------------------------------------------------------*/
/**
 * Definitions for the control-message based 
 * shm communication between libnts and ntb-proxy.
 */
struct nts_ntp_context {

	pthread_t shm_recv_thr;
	int shm_recv_signal;

};

typedef struct nts_ntp_context* nts_ntp_context_t;


/*----------------------------------------------------------------------------*/
/**
 * Definitions for the epoll manager context.
 */
typedef struct nt_epoll_context {
	nt_socket_t socket;

	/**
	 * The nts epoll shm channel between epoll and ntm
	 * 
	 * 1. init or create first when invoking `epoll_create()`
	 * 
	 */
	char epoll_shmaddr[EPOLL_SHM_NAME_LEN];
	int epoll_shmlen;
	int epoll_msg_id;
	epoll_shm_context_t epoll_shm_ctx;

	// for SHM-based ready I/O queue,
	// ntm generates the `io_queue_shmaddr` and return to libnts
	char io_queue_shmaddr[EPOLL_SHM_NAME_LEN];
	int io_queue_shmlen;
	int io_queue_size;


	// cache all focused socket: listen socket or client socket
	/**
	 * key: nt_socket_id
	 * value: nt_socket_t
	 */
	HashMap epoll_sockets;

} nt_epoll_context;

typedef struct nt_epoll_context* nt_epoll_context_t;


/*----------------------------------------------------------------------------*/
/**
 * Definitions of ntm_manager to manage the context of libnts
 */
struct nts_context {
	nts_ntm_context_t ntm_ctx;

	nts_ntp_context_t ntp_ctx;

	/**
	 * hold the global nt_socket list 
	 * 
	 * key: nt_socket_id
	 * value: nt_sock_context_t
	 */
	HashMap nt_sock_map;

	/** 
	 * hold the process-level global nt_epoll_context_t list
	 * 
	 * key: epoll nt_socket_id
	 * value: nt_epoll_context_t
	 */
	HashMap nt_epoll_map;

	/**
	 * nts init flag:
	 * 1 when nts init, else 0
	 */
	uint8_t init_flag;

	/**
	 * nts exit flag:
	 * w when nts exit, else 0
	 */
	uint8_t exit;
	
	// for FD remmaping table
	// FD remapping table, maintain nt_socket_id in this remapping table
	// key: sock_fd
	// value: NULL
	HashMap fd_table;

} nts_context;

typedef struct nts_context* nts_context_t;

/*----------------------------------------------------------------------------*/
extern nts_context_t nts_ctx;
extern struct nts_config NTS_CONFIG;


/*----------------------------------------------------------------------------*/
/* Related methods definitions for libnts */

nts_context_t get_nts_context();


int nts_context_init(const char *config_file);

void nts_context_destroy();

/**
 * generate unique nts shm name
 * input: shm addr memory address
 * output: unique nts shm name
 * 
 * if return 0, success
 * else return -1, failed
 */
int generate_nts_shmname(char * nts_shmaddr);

/**
 * @brief  generate unique epoll shm name for the msg ringbuffer 
 * 			between libnts and ntm.
 * @note   
 * @param  epoll_shmaddr: 
 * @retval if 0, success; else if -1, failed.
 */
int generate_epoll_shmname(char * epoll_shmaddr);


int nt_sock_errno(int sockid);


#ifdef __cplusplus
};
#endif

#endif /* NTS_H_ */
