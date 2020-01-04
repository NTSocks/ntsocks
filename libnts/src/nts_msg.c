/*
 * <p>Title: nts_msg.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#include <string.h>
#include <assert.h>

#include "nts_msg.h"

void nts_msgcopy(nts_msg *src_msg, nts_msg *target_msg) {
    assert(src_msg);
    assert(target_msg);

    target_msg->msg_id = src_msg->msg_id;
    target_msg->msg_type = src_msg->msg_type;

    if(src_msg->addrlen > 0) {
        memcpy(target_msg->address, src_msg->address, src_msg->addrlen);
        target_msg->addrlen = src_msg->addrlen;
        target_msg->port = src_msg->port;
    }

    target_msg->sockid = src_msg->sockid;

}

