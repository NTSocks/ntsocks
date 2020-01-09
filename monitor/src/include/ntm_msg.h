/*
 * <p>Title: ntm_msg.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef NTM_MSG_H_
#define NTM_MSG_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t ntm_msg_id;
typedef uint64_t nts_msg_id;
typedef uint64_t ntsock_id;



/*----------------------------------------------------------------------------*/
/**
 * the message definition for the communication
 * between local and remote ntb-monitor.
 */

#define PAYLOAD_SIZE 1024

typedef enum {
    CONNECT_OK = 1,
    TRACK = 10,
    TRACK_CONFIRM = 15,
    STOP = 20,
    STOP_CONFIRM = 25,
    SUCCESS = 30,
    FAILURE = 40,
    QUIT = 50,

    NT_SYN = 60,
    NT_SYN_ACK = 61,
    NT_INVALID_PORT = 62,
    NT_LISTENER_NOT_FOUND = 63,
    NT_LISTENER_NOT_READY = 64,
    NT_BACKLOG_IS_FULL = 65

} ntm_sock_msg_type;

typedef struct {
    unsigned int src_addr;          // source ip address
    unsigned int dst_addr;          // destination address
    unsigned short sport;           // source port
    unsigned short dport;           // destination port
	ntm_sock_msg_type type;
    char payload[PAYLOAD_SIZE];
} ntm_sock_msg;

typedef struct {
    int human_id;
    int torso_height;
    int arm_length;
    int leg_length;
    int head_diameter;
} human_data;

// typedef struct {
//     ntm_msg_id msg_id;
//     ntsock_id sockid; 


// } nt_syn;



/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */
#define SHM_NAME_LEN 128

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
    NTM_MSG_CONNECT = 1 << 5,
    NTM_MSG_ESTABLISH = 1 << 6,
    NTM_MSG_CLOSE = 1 << 7,
    NTM_MSG_DISCONNECT = 1 << 8, 
    NTM_MSG_SHUTDOWN = 1 << 9,
    NTM_MSG_ERR = 1 << 10
} ntm_msg_type;


typedef struct {
    ntm_msg_id msg_id;
    ntm_msg_type msg_type;
    ntsock_id sockid;

    /**
     * For NTM_MSG_NEW_SOCK
     */
    int nts_shm_addrlen;
    char nts_shm_name[SHM_NAME_LEN];
    int domain;
    int sock_type;
    int protocol;

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
    int backlog;

    /**
     * For NTM_MSG_ACCEPT
     * input: server socket id, req_client_sockaddr(1, 0)
     * output: client socket id, (if req_client_sockaddr is 1, 
     *      return client ip address and port)
     */
    // ntsock_id sockid;
    uint8_t req_client_sockaddr; // if 1, return client addr:port; else, not

    // if req_client_sockaddr == 1
    // char address[16];
    // uint32_t addrlen;
    // uint64_t port;


    /**
     * For NTM_MSG_CONNECT
     */
    // sockid
    char address[16];
    uint8_t addrlen;
    uint16_t port;


    /**
     * For NTM_MSG_CLOSE
     */
    // socket id

    /**
     * For NTM_MSG_DISCONNECT
     */

     /**
     * For NTM_MSG_SHUTDOWN
     */
    // socket id
    shutdown_type howto; // 

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
    NTS_MSG_DISCONNECT = 1 << 11,
    NTS_MSG_SHUTDOWN = 1 << 12,
    NTS_MSG_ERR = 1 << 13
} nts_msg_type;

typedef enum {
    NT_SUCCESS = 1 << 0,
    NT_FAILED = 1 << 1,
    NT_DISCONNECT = 1 << 2
} nt_conn_status;


typedef struct {
    nts_msg_id msg_id;
    nts_msg_type msg_type;
    ntsock_id sockid;
    int retval; // if 0, pass; else -1, failed
    int nt_errno;  // conresponding to error type

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
    // client socket id = ntsock_id sockid;
    ntsock_id client_sockid;

    // if req_client_sockaddr == 1
    // char address[16];
    // uint32_t addrlen;
    // uint64_t port;

    /**
     * For NTS_MSG_CONNECT
     */
    nt_conn_status conn_status;
    // retval
    // nt_errno

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


    char address[16];
    uint32_t addrlen;
    uint64_t port;

   
} nts_msg;

void nts_msgcopy(nts_msg *src_msg, nts_msg *target_msg);


#ifdef __cplusplus
};
#endif

#endif /* NTM_MSG_H_ */
