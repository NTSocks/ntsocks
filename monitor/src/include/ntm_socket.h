/*
 * <p>Title: ntm_socket.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 29, 2019 
 * @version 1.0
 */

#ifndef NTM_SOCKET_H_
#define NTM_SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LISTEN_QUEUE_NUM 255
#define BUFLEN 256

typedef struct ntm_socket {
    int socket_fd;
    struct sockaddr_in remote;
    struct sockaddr_in local;
    char buf[BUFLEN];
    char retbuf[BUFLEN];
    socklen_t len;
    socklen_t rlen;
    struct hostent *hp;
} ntm_socket;

typedef struct ntm_socket* ntm_socket_t;


ntm_socket_t ntm_sock_create();

/**
 * Create a socket given a file descriptor
 * @param fd
 * @return
 */
ntm_socket_t ntm_client_create(int fd);

int ntm_free(ntm_socket_t ntsock);

/**
 * Send a TCP message on socket
 * @param ntsock
 * @param msg
 * @param msg_size
 * @return int
 */
int ntm_send_tcp_msg(ntm_socket_t ntsock, char *msg, int msg_size);


/**
 * Receive a TCP message on socket.
 * @param ntsock
 * @param buf
 * @param buf_size
 * @return message length
 */
int ntm_recv_tcp_msg(ntm_socket_t ntsock, char *buf, int buf_size);

/**
 * Start a TCP server on given port and address
 * @param ntsock
 * @param port
 * @param address
 * @return port TCP server is running on
 */
int ntm_start_tcp_server(ntm_socket_t ntsock, int port, char *address);

/**
 * Waits for a TCP connection and creates a new socket
 * @param ntsock
 * @return
 */
ntm_socket_t ntm_accept_tcp_conn(ntm_socket_t ntsock);

/**
 * Connects to a TCP server on given port and hostname
 * @param ntsock
 * @param port
 * @param name
 * @return
 */
int ntm_connect_to_tcp_server(ntm_socket_t ntsock, int port, char *name);

/**
 * closes a socket
 * @param ntsock
 * @return
 */
int ntm_close_socket(ntm_socket_t ntsock);


#ifdef __cplusplus
};
#endif

#endif /* NTM_SOCKET_H_ */
