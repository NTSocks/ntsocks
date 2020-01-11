/*
 * <p>Title: libnts.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Nov 29, 2019 
 * @version 1.0
 */

#ifndef LIBNTS_H_
#define LIBNTS_H_

#include <linux/ioctl.h>


typedef unsigned int u32;


struct ntsocket_ioctl_arg {
	u32 fd;
	u32 backlog;

	union ops_arg {
		struct socket_accept_op_t {
			void *sockaddr;
			int *sockaddr_len;
			int flags;
		}accept_op;

		struct spawn_op_t {
			int cpu;
		}spawn_op;

		struct io_op_t {
			char *buf;
			u32 buf_len;
		}io_op;

		struct socket_op_t {
			u32 family;
			u32 type;
			u32 protocol;
		}socket_op;

		struct shutdown_op_t {
			int how;
		}shutdown_op;

		struct epoll_op_t {
			u32 epoll_fd;
			u32 size;
			u32 ep_ctl_cmd;
			u32 time_out;
			struct epoll_event *ev;
		}epoll_op;
	} op;

};






#endif /* LIBNTS_H_ */
