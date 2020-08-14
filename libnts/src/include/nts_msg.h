/*
 * <p>Title: nts_msg.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef NTS_MSG_H_
#define NTS_MSG_H_

#include <stdint.h>
#include <stdbool.h>

#include "socket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t ntm_msg_id;
typedef uint64_t nts_msg_id;


/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */
#define SHM_NAME_LEN 64

typedef enum {
    NT_SHUT_RD = 1,  // close socket read operation
    NT_SHUT_WR = 2,  // close socket write operation
    NT_SHUT_RDWR = 3 // close socket read/write operations
} shutdown_type;

typedef enum {
    NTM_MSG_INIT = 1 << 0,
    NTM_MSG_NEW_SOCK = 1 << 1,
    NTM_MSG_BIND = 1 << 2,
    NTM_MSG_LISTEN = 1 << 3,
    NTM_MSG_ACCEPT = 1 << 4,
    NTM_MSG_ACCEPT_ACK = 1 << 5,
    NTM_MSG_CONNECT = 1 << 6,
    NTM_MSG_ESTABLISH = 1 << 7,
    NTM_MSG_CLOSE = 1 << 8,
    NTM_MSG_FIN = 1 << 9,               // for passively close/FIN
    NTM_MSG_SHUTDOWN = 1 << 10,
    NTM_MSG_ERR = 1 << 11,
    NTM_MSG_DISCONNECT = 1 << 12,
    NTM_MSG_EPOLL_CREATE = 1 << 13,
    NTM_MSG_EPOLL_CTL = 1 << 14,
    NTM_MSG_EPOLL_CLOSE = 1 << 15
} ntm_msg_type;


typedef struct {
    ntm_msg_id msg_id;
    ntm_msg_type msg_type;
    int sockid;
    shutdown_type howto; 
    int backlog;
    int req_client_sockaddr;
    int domain;
    int sock_type;
    int io_queue_size;  // for epoll ready I/O queue size
    int protocol;
    int nts_shm_addrlen;
    int addrlen;
    int port;
    int epid;   // for epoll socket id, NTM_MSG_EPOLL_CTL
    int epoll_op;   // NTS_EPOLL_CTL_ADD, NTS_EPOLL_CTL_MOD, NTS_EPOLL_CTL_DEL
    uint32_t events;    // NTM_MSG_EPOLL_CTL
    uint64_t ep_data;   // NTM_MSG_EPOLL_CTL
    char address[16];
    char nts_shm_name[SHM_NAME_LEN];

    /**
     * For NTM_MSG_NEW_SOCK
     */
    // int nts_shm_addrlen;
    // char nts_shm_name[SHM_NAME_LEN];
    // int domain;
    // int sock_type;
    // int protocol;
    

    /**
     * For NTM_MSG_BIND
     */
    // Default sin_family AF_INET
    // char address[16];
    // uint8_t addrlen;
    // uint64_t port;

    /**
     * For NTM_MSG_LISTEN
     * backlog: the upper limit of nt client connection
     */
    // int backlog;

    /**
     * For NTM_MSG_ACCEPT
     * input: server socket id, req_client_sockaddr(1, 0)
     * output: client socket id, (if req_client_sockaddr is 1, 
     *      return client ip address and port)
     */
    // int sockid;
    // uint8_t req_client_sockaddr; // if 1, return client addr:port; else, not

    // if req_client_sockaddr == 1
    // char address[16];
    // uint32_t addrlen;
    // uint64_t port;

    /**
     * For NTM_MSG_ACCEPT_ACK
     */
    // nt_sock_id sockid;
    // int nts_shm_addrlen;
    // char nts_shm_name[SHM_NAME_LEN];


    /**
     * For NTM_MSG_CONNECT
     */
    // sockid
    // char address[16];
    // uint8_t addrlen;
    // uint16_t port;


    /**
     * For NTM_MSG_CLOSE
     */
    // socket id

    /**
     * For NTM_MSG_FIN
     */
    // socket id

     /**
     * For NTM_MSG_SHUTDOWN
     */
    // socket id
    // shutdown_type howto; 

    /**
     * For NTM_MSG_EPOLL_CREATE
     */
    // socket id
    // sock_type
    // io_queue_size
    // nts_shm_addrlen
    // nts_shm_name

    /**
     * For NTM_MSG_EPOLL_CTL
     */
    // socket id
    // sock_type for client or listen socket
    // epid for epoll socket id
    // events
    // ep_data
    // epoll_op

    /**
     * For NTM_MSG_ERR
     */

} ntm_msg;

void ntm_msgcopy(ntm_msg *src_msg, ntm_msg *target_msg);



/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */
typedef enum {
    NTS_MSG_INIT = 1 << 0,
    NTS_MSG_NEW_SOCK = 1 << 1,
    NTS_MSG_BIND = 1 << 2,
    NTS_MSG_LISTEN = 1 << 3,
    NTS_MSG_ACCEPT = 1 << 4,
    NTS_MSG_CONNECT = 1 << 5,
    NTS_MSG_DISPATCHED = 1 << 6,
    NTS_MSG_ESTABLISH = 1 << 7,
    NTS_MSG_CLOSE = 1 << 8,
    NTS_MSG_FIN = 1 << 9,               // for actively socket close/FIN
    NTS_MSG_DISCONNECT = 1 << 11,
    NTS_MSG_SHUTDOWN = 1 << 12,
    NTS_MSG_ERR = 1 << 13,
    NTS_MSG_CLIENT_SYN_ACK = 1 << 14
    
} nts_msg_type;

typedef enum {
    NT_SUCCESS = 1 << 0,
    NT_FAILED = 1 << 1,
    NT_DISCONNECT = 1 << 2
} nt_conn_status;


typedef struct {
    nts_msg_id msg_id;
    nts_msg_type msg_type;
    int sockid;
    int retval; // if 0, pass; else -1, failed
    int nt_errno;  // conresponding to error type
    nt_conn_status conn_status;
    int addrlen;
    int port;   
    char address[16];
    

    /**
     * For NTS_MSG_INIT
     */


    /**
     * For NTS_MSG_NEW_SOCK
     */
    // retval
    // nt_errno
    // socket id

    /**
     * For NTS_MSG_BIND
     */
    // retval
    // nt_errno

    /**
     * For NTS_MSG_LISTEN
     */
    // retval


     /**
     * For NTS_MSG_ACCEPT
     */
    // client socket id = int sockid;
    // int client_sockid;

    // if req_client_sockaddr == 1
    // char address[16];
    // uint32_t addrlen;
    // uint64_t port;

    /**
     * For NTS_MSG_CONNECT
     */
    // nt_conn_status conn_status;
    // retval
    // nt_errno

    /**
     * For NTS_MSG_DISPATCHED
     */ 
     // retval
    // nt_errno
    // port  // client socket nt_port

    /**
     * For NTS_MSG_CLOSE
     */
    // retval
    // monitor status ?
    // conn_status ?
    // close socket write/read

    /**
     * For NTS_MSG_ESTABLISH
     */
    // sockid

    /**
     * For NTS_MSG_DISCONNECT
     */

    /**
     * For NTS_MSG_SHUTDOWN
     */
    // retval
   
} nts_msg;

void nts_msgcopy(nts_msg *src_msg, nts_msg *target_msg);


#ifdef __cplusplus
};
#endif

#endif /* NTS_MSG_H_ */
