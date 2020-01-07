/*
 * <p>Title: socket.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#include <stdlib.h>

#include "socket.h"

nt_socket_t allocate_socket(int socktype, int need_lock) {
	struct nt_socket new_socket;

	new_socket.sockid = 12;
	new_socket.socktype = socktype;
	new_socket.saddr.sin_family = AF_INET;
	new_socket.saddr.sin_port = htons(8080);
	new_socket.saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	new_socket.state = CLOSED;

	return &new_socket;
}

void free_socket(int sockid, int need_lock) {

}

nt_socket_t get_socket(int sockid) {

	return NULL;
}

nt_port allocate_port(int need_lock) {

	return 9092;
}

bool is_idle_port(int port) {

	return true;
}

void free_port(int port, int need_lock) {

}