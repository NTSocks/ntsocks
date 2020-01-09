
#ifndef NTM_MONITOR_H
#define NTM_MONITOR_H


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <netinet/in.h>

#include "nt_log.h"
#include "ntm_shm.h"
#include "nts_shm.h"
#include "hash_map.h"
#include "ntm_msg.h"
#include "socket.h"
#include "ntm_socket.h"
#include "nt_backlog.h"

#ifdef __cplusplus
extern "C" {
#endif


#define NTM_SHMRING_NAME "/ntm-shm-ring"


/*----------------------------------------------------------------------------*/
struct ntm_config {

	/* config for the connection between local and remote nt-monitor  */
	int remote_ntm_tcp_timewait;
	int remote_ntm_tcp_timeout;

	char *listen_ip;
	int listen_port;
	int ipaddr_len;

	int nt_max_conn_num;    // max number of ntb-based socket connections
	int nt_max_port_num;	// max number of ntb-based port

};

typedef struct ntm_conn * ntm_conn_t;


/*----------------------------------------------------------------------------*/
/**
 * Definitions for the shm communication between nt-monitor and nts app.
 */

struct nts_shm_conn {
	nt_sock_id sockid;
	nt_socket_t socket;
	int domain;
    int protocol;

	int sock_status;
	bool is_listen;
	bool running_signal; // if 1, running; else stop

	int port;
	int addrlen;
	char ip[16];

	int shm_addrlen;
	char nts_shm_name[SHM_NAME_LEN];
	nts_shm_context_t nts_shm_ctx;

	ntm_conn_t ntm_conn;
};

typedef struct nts_shm_conn * nts_shm_conn_t;

struct ntm_nts_context {
	/**
	 * Cache the nts shm ring buffer contexts for ntb-monitor.
	 * When the nts app invoke socket() to request to ntb-monitor
	 * at the first time, ntb-monitor receives the nts shm name,
	 *  allocates socket, push the <socket, nts_shm_ctx> into hashmap,
	 *  and send the socket back to nts app.
	 *
	 * key:  & socket_id (memory address)
	 * value: nts_shm_conn_t
	 */
	HashMap nts_shm_conn_map;
	
	/**
	 *	for ntm shm receive queue
	 */
	ntm_shm_context_t shm_recv_ctx;
	pthread_t shm_recv_thr;
	int shm_recv_signal;

	/**
	 * for ntm shm send queue
	 */
	nts_shm_context_t shm_send_ctx;
	pthread_t shm_send_thr;
	int shm_send_signal;

};

typedef struct ntm_nts_context* ntm_nts_context_t;


/*----------------------------------------------------------------------------*/
/**
 * Definitions for the shm communication between nt-monitor and ntb-proxy.
 */
struct ntm_ntp_context {

};

typedef struct ntm_ntp_context* ntm_ntp_context_t;




/*----------------------------------------------------------------------------*/
/**
 * Definitions for the socket communication between local ntb-monitor and remote ntb-monitor.
 */
struct ntm_conn {
    char ip[16];
    int addrlen;
    int sockfd;
    int port;
    bool running_signal; // if 1, running; else stop
    ntm_socket_t client_sock;
    pthread_t recv_thr;
};

//typedef struct ntm_conn *ntm_conn_t;

struct ntm_conn_context {
	/**
	 * Cache the remote nt-monitor connection list
	 *
	 * key: IP Address of remote ntb-monitor
	 * value: the memory pointer of ntm_conn_t
	 */
	HashMap conn_map;

	ntm_socket_t server_sock;
	pthread_t ntm_sock_listen_thr;
	int running_signal; // if 1, running; else stop
	int addrlen;
	int listen_port;
	char *listen_ip;

};

typedef struct ntm_conn_context *ntm_conn_ctx_t;


/*----------------------------------------------------------------------------*/
/**
 * Definitions of nt_listener for the ntb-based listen socket.
 *  1. differentiate the connect-based client socket and listener socket driven
 *  	by ntb devices, specifically defined for ntb-based listener socket.
 *  2. hold the accepted client socket from remote ntm for local specific nt_listener
 *	3. map the specified `port` into corresponding nt_listener
 *	4. forward the accepted remote connection requests to corresponding nt_listener
 *
 */
struct nt_accepted_conn {
	nt_sock_id sockid;
	nt_socket_t client_sock;

	ntm_conn_t ntm_conn;

	nt_listener_t listener;  // point the corresponding nt_listener pointer

};
typedef struct nt_accepted_conn * nt_accepted_conn_t;

#define BACKLOG_SHMLEN 30
struct nt_listener_wrapper {

	nt_listener_t listener;
	int port;
	int addrlen;
	char ip[16];

	/**
	 * backlog context:
	 * 	hold the established client nt_socket list.
	 * 	the max capacity of backlog is determined by the backlog in nt_listener.
	 * 
	 * when received established client nt_socket, push it into backlog queue.
	 * Meanwhile, the listening nts endpoint consumes the established client nt_socket
	 * 	by invoking `accept()`.
	 */
	nt_backlog_context_t backlog_ctx;
	char backlog_shmaddr[BACKLOG_SHMLEN];	// backlog shm name, default rule: backlog-{sock_id}
	size_t backlog_shmlen;	

	/**
	 * hold the client socket accepted by current listener
	 *
	 * key: & client socket id (memory address)
	 * value: nt_accepted_conn_t
	 */
	HashMap accepted_conn_map;

	nts_shm_conn_t nts_shm_conn;  // point the related nts shm context pointer

};
typedef struct nt_listener_wrapper * nt_listener_wrapper_t;


struct nt_listener_context {
	/**
	 * hold all nt_listener indexed by service `port`
	 *
	 * key: port
	 * value: nt_listener_wrapper_t
	 */
	HashMap listener_map;

};
typedef struct nt_listener_context * nt_listener_context_t;


/*----------------------------------------------------------------------------*/
/**
 * Definitions of ntm_manager to manage the context information of nt-monitor
 */
struct ntm_manager {
	ntm_nts_context_t nts_ctx;
	ntm_ntp_context_t ntp_ctx;

	ntm_conn_ctx_t ntm_conn_ctx;

	nt_listener_context_t nt_listener_ctx;


	/**
	 * hold all (connect) client nt_socket indexed by client `port`
	 * 
	 * key: port
	 * value: nt_socket_t
	 */
	HashMap port_sock_map;

};

typedef struct ntm_manager* ntm_manager_t;


/*----------------------------------------------------------------------------*/
/**
 * Definitions of global variables for ntb-monitor
 */
extern struct ntm_config NTM_CONFIG;
extern ntm_manager_t ntm_mgr;



/*----------------------------------------------------------------------------*/
/* Related methods definitions for NTB Monitor */

ntm_manager_t get_ntm_manager();

/**
 * 0. read the related conf file;
 * 1. init ntm related resources, such as available socket list;
 * 2. init the ntm shm ringbuffer to receive the messages from libnts apps;
 * 3. init the listen socket for the connection requests from remote nt-monitor;
 * 4. init the connection status hashmap store for remote nt-monitor;
 *
 */
int ntm_init(const char *config_file);

void ntm_destroy();

int ntm_getconf(struct ntm_config *conf);

int ntm_setconf(const struct ntm_config *conf);

ntm_manager_t init_ntm_manager();



/**
 * Define the loop function for ntb-monitor server listening socket
 * to receive the remote ntb-monitor connection
 *
 * input: ntm_conn_ctx_t
 */
void * ntm_sock_listen_thread(void *args);

/**
 * Define the loop function for accepted remote ntb-monitor client socket
 * to callback the handler functions to response the requests.
 *
 * input: ntm_conn_t
 * sender: remote ntb-monitor
 * receiver: local ntb-monitor
 */
void * ntm_sock_recv_thread(void *args);

/**
 * the callback handler function for the request messages
 * from the remote ntb-monitor
 *
 * sender: remote ntb-monitor
 * receiver: local ntb-monitor
 */
void ntm_sock_handle_msg(ntm_conn_t ntm_conn, ntm_sock_msg msg);


/**
 * the receive loop function for ntm_nts_ringbuffer to
 * async receive and handle the shm messages from nts app
 *
 * input: ntm_nts_context_t
 * sender: nts app
 * receiver: ntb-monitor
 */
void * nts_shm_recv_thread(void *args);

/**
 * the callback handler function for the request messages
 * from the local nts app
 *
 * sender: nts app
 * receiver: ntb-monitor
 */
void nts_shm_handle_msg(ntm_manager_t ntm_mgr, ntm_msg msg);


int print_monitor();


#ifdef __cplusplus
};
#endif

#endif // end NTM_MONITOR_H 
