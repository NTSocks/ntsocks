/*
 * <p>Title: ntp_msg.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang, Jing7
 * @date Dec 29, 2019 
 * @version 1.0
 */

#include <string.h>
#include <assert.h>

#include "ntp_msg.h"

void ntp_msgcopy(ntp_msg *src_msg, ntp_msg *target_msg) {
	assert(src_msg);
	assert(target_msg);

	target_msg->msg_type = src_msg->msg_type;

	if (src_msg->msg_len > 0) {
		memcpy(target_msg->msg, src_msg->msg, src_msg->msg_len);
		target_msg->msg_len = src_msg->msg_len;
	}
}

// void ntp_msgcopy(ntp_msg *src_msg, ntp_msg *target_msg) {
// 	assert(src_msg);
// 	assert(target_msg);

// 	target_msg->msg_id = src_msg->msg_id;
// 	target_msg->msg_type = src_msg->msg_type;

// 	if (src_msg->addrlen > 0) {
// 		memcpy(target_msg->address, src_msg->address, src_msg->addrlen);
// 		target_msg->addrlen = src_msg->addrlen;
// 		target_msg->port = src_msg->port;
// 	}

// 	target_msg->sockid = src_msg->sockid;
// }

