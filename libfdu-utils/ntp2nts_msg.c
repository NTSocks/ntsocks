/*
 * <p>Title: ntp2nts_msg.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang, Jing7
 * @date Dec 29, 2019 
 * @version 1.0
 */

#include <string.h>
#include <assert.h>

#include "ntp2nts_msg.h"
#include "utils.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);


void ntp_msgcopy(ntp_msg *src_msg, ntp_msg *target_msg) {
	assert(src_msg);
	assert(target_msg);

	target_msg->msg_type = src_msg->msg_type;
	if (LIKELY(src_msg->msg_len > 0)) {
		memcpy(target_msg->msg, src_msg->msg, src_msg->msg_len);
		target_msg->msg_len = src_msg->msg_len;
	} else {
		ERR("Invalid src_msg with src_msg->msg_len <= 0");
	}
}

