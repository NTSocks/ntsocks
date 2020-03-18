/*
 * <p>Title: ntm_msg.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 29, 2019 
 * @version 1.0
 */

#include <string.h>
#include <assert.h>
#include "ntm_msg.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

void ntm_ntp_msgcopy(ntm_ntp_msg *src_msg, ntm_ntp_msg *target_msg)
{
	assert(src_msg);
	assert(target_msg);

	target_msg->src_ip = src_msg->src_ip;
	target_msg->dst_ip = src_msg->dst_ip;
	target_msg->src_port = src_msg->src_port;
	target_msg->dst_port = src_msg->dst_port;
	target_msg->msg_type = src_msg->msg_type;
	target_msg->msg_len = src_msg->msg_len;
	if (src_msg->msg_len > 0)
	{
		memcpy(target_msg->msg, src_msg->msg, src_msg->msg_len);
	}
	DEBUG("src_msg->msg_type=%d, target_msg->msg_type=%d", src_msg->msg_type, target_msg->msg_type);
}

void ntp_ntm_msgcopy(ntp_ntm_msg *src_msg, ntp_ntm_msg *target_msg)
{
	assert(src_msg);
	assert(target_msg);

	target_msg->src_ip = src_msg->src_ip;
	target_msg->dst_ip = src_msg->dst_ip;
	target_msg->src_port = src_msg->src_port;
	target_msg->dst_port = src_msg->dst_port;
	target_msg->msg_type = src_msg->msg_type;
	target_msg->msg_len = src_msg->msg_len;
	if (src_msg->msg_len > 0)
	{
		memcpy(target_msg->msg, src_msg->msg, src_msg->msg_len);
	}
	DEBUG("src_msg->msg_type=%d, target_msg->msg_type=%d", src_msg->msg_type, target_msg->msg_type);
}
