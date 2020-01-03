/*
 * <p>Title: ntm_socket.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 29, 2019 
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "ntm_socket.h"

ntm_socket_t ntm_sock_create() {
	ntm_socket_t ntsock;

    ntsock = (ntm_socket_t)calloc(1, sizeof(struct ntm_socket));
    if (!ntsock) {
        perror("malloc");
        printf("failed to allocate nt_socket.\n");
        return NULL;
    }

    ntsock->len = sizeof(ntsock->local);
    ntsock->rlen = sizeof(ntsock->remote);

    return ntsock;
}

ntm_socket_t ntm_client_create(int fd) {
    assert(fd >= 0);
    ntm_socket_t ntsock;

    ntsock = (ntm_socket_t)calloc(1, sizeof(struct ntm_socket));
    if (!ntsock) {
        perror("malloc");
        printf("failed to allocate nt_socket.\n");
        return NULL;
    }

    ntsock->socket_fd = fd;
    ntsock->len = sizeof(ntsock->local);
    ntsock->rlen = sizeof(ntsock->remote);

    return ntsock;
}

int ntm_free(ntm_socket_t ntsock) {
    int ret;
    ret = ntm_close_socket(ntsock);

    return ret;
}

int ntm_send_tcp_msg(ntm_socket_t ntsock, char *msg, int msg_size) {
    assert(ntsock);
    assert(msg);
    assert(msg_size > 0);

    int ret;
    ret = (int) send(ntsock->socket_fd, msg, msg_size, 0);

    return ret;
}

int ntm_recv_tcp_msg(ntm_socket_t ntsock, char *buf, int buf_size) {
    assert(ntsock);
    assert(buf);
    assert(buf_size > 0);

    int msglen;
    msglen = (int) recv(ntsock->socket_fd, buf, buf_size, 0);

    return msglen;
}

int ntm_start_tcp_server(ntm_socket_t ntsock, int port, char *address) {
    assert(ntsock);
    assert(address);

    // create socket
    ntsock->socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // setup socket
    ntsock->local.sin_family = AF_INET;
    ntsock->hp = gethostbyname(address);
//    ntsock->local.sin_addr.s_addr = (in_addr_t) address;
    memcpy(&ntsock->local.sin_addr, ntsock->hp->h_addr_list[0], (size_t) ntsock->hp->h_length);
    ntsock->local.sin_port = (in_port_t) port;

    // bind socket to addr
    bind(ntsock->socket_fd, (const struct sockaddr *) &ntsock->local, ntsock->len);
    listen(ntsock->socket_fd, LISTEN_QUEUE_NUM);

    // return port number
    getsockname(ntsock->socket_fd, (struct sockaddr *) &ntsock->local, &ntsock->len);

    return ntsock->local.sin_port;
}

ntm_socket_t ntm_accept_tcp_conn(ntm_socket_t ntsock) {
    assert(ntsock);

    ntm_socket_t new_socket;
    int accept_conn_fd;
    new_socket = ntm_sock_create();

    accept_conn_fd = accept(ntsock->socket_fd, (struct sockaddr *) &new_socket->remote, &new_socket->rlen);
    if (accept_conn_fd != -1) {
        new_socket->socket_fd = accept_conn_fd;
        return new_socket;
    }

    ntm_close_socket(new_socket);
    return NULL;
}

int ntm_connect_to_tcp_server(ntm_socket_t ntsock, int port, char *name) {
    assert(ntsock);
    int ret;

    // create socket
    ntsock->socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // setup socket
    ntsock->remote.sin_family = AF_INET;
    ntsock->hp = gethostbyname(name);
    memcpy(&ntsock->remote.sin_addr, ntsock->hp->h_addr_list[0], (size_t) ntsock->hp->h_length);
    ntsock->remote.sin_port = (in_port_t) port;

    // connect to server
    ret = connect(ntsock->socket_fd, (const struct sockaddr *) &ntsock->remote, ntsock->rlen);
    if (ret < 0) {
        close(ntsock->socket_fd);
        return -1;
    }

    return 0;
}


int ntm_close_socket(ntm_socket_t ntsock) {
    assert(ntsock);

    if (ntsock->socket_fd >= 0) {
       close(ntsock->socket_fd);
    }

    free(ntsock);

    return 0;
}

