#include <string.h>
#include <assert.h>

#include "nts_epoll.h"
#include "ntm_msg.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

void ntm_msgcopy(ntm_msg *src_msg, ntm_msg *target_msg)
{
	assert(src_msg);
	assert(target_msg);

	target_msg->msg_id = src_msg->msg_id;
	target_msg->msg_type = src_msg->msg_type;
	DEBUG("ntm_msgcopy: sockid=%d, msg_type=%d",
		  src_msg->sockid, src_msg->msg_type);

	if (src_msg->sockid > 0)
	{
		target_msg->sockid = src_msg->sockid;
	}

	if (target_msg->msg_type & NTM_MSG_INIT)
	{
	}
	else if (target_msg->msg_type & NTM_MSG_NEW_SOCK)
	{
		target_msg->nts_shm_addrlen = src_msg->nts_shm_addrlen;
		memcpy(target_msg->nts_shm_name, src_msg->nts_shm_name, src_msg->nts_shm_addrlen);

		target_msg->domain = src_msg->domain;
		target_msg->protocol = src_msg->protocol;
		target_msg->sock_type = src_msg->sock_type;
	}
	else if (target_msg->msg_type & NTM_MSG_BIND)
	{
		target_msg->sockid = src_msg->sockid;
		memcpy(target_msg->address, src_msg->address, src_msg->addrlen);
		target_msg->addrlen = src_msg->addrlen;
		target_msg->port = src_msg->port;
	}
	else if (target_msg->msg_type & NTM_MSG_LISTEN)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->backlog = src_msg->backlog;
	}
	else if (target_msg->msg_type & NTM_MSG_ACCEPT)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->req_client_sockaddr = src_msg->req_client_sockaddr;
	}
	else if (target_msg->msg_type & NTM_MSG_ACCEPT_ACK)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->nts_shm_addrlen = src_msg->nts_shm_addrlen;
		memcpy(target_msg->nts_shm_name, src_msg->nts_shm_name, src_msg->nts_shm_addrlen);
	}
	else if (target_msg->msg_type & NTM_MSG_CONNECT)
	{
		target_msg->sockid = src_msg->sockid;
		memcpy(target_msg->address, src_msg->address, src_msg->addrlen);
		target_msg->addrlen = src_msg->addrlen;
		target_msg->port = src_msg->port;
	}
	else if (target_msg->msg_type & NTM_MSG_CLOSE)
	{
		target_msg->sockid = src_msg->sockid;
	}
	else if (target_msg->msg_type & NTM_MSG_SHUTDOWN)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->howto = src_msg->howto;
	}
	else if (target_msg->msg_type & NTM_MSG_FIN)
	{
		target_msg->sockid = src_msg->sockid;
	}
	else if (target_msg->msg_type & NTM_MSG_EPOLL_CREATE)
	{
		target_msg->nts_shm_addrlen = src_msg->nts_shm_addrlen;
		memcpy(target_msg->nts_shm_name,
			   src_msg->nts_shm_name, src_msg->nts_shm_addrlen);
		target_msg->sock_type = src_msg->sock_type;
		target_msg->io_queue_size = src_msg->io_queue_size;
	}
	else if (target_msg->msg_type & NTM_MSG_EPOLL_CTL)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->sock_type = src_msg->sock_type;
		target_msg->epid = src_msg->epid;
		target_msg->epoll_op = src_msg->epoll_op;
		if (src_msg->epoll_op != NTS_EPOLL_CTL_DEL)
		{
			target_msg->events = src_msg->events;
			target_msg->ep_data = src_msg->ep_data;
		}
	}
	else if (target_msg->msg_type & NTM_MSG_EPOLL_CLOSE)
	{
		target_msg->epid = src_msg->epid;
		target_msg->sock_type = src_msg->sock_type;
	}
}

void nts_msgcopy(nts_msg *src_msg, nts_msg *target_msg)
{
	assert(src_msg);
	assert(target_msg);

	target_msg->msg_id = src_msg->msg_id;
	target_msg->msg_type = src_msg->msg_type;

	if (src_msg->sockid > 0)
	{
		target_msg->sockid = src_msg->sockid;
	}

	if (target_msg->msg_type & NTS_MSG_INIT)
	{
	}
	else if (target_msg->msg_type & NTS_MSG_NEW_SOCK)
	{
		target_msg->retval = src_msg->retval;
		target_msg->nt_errno = src_msg->nt_errno;
		// target_msg->sockid = src_msg->sockid;
	}
	else if (target_msg->msg_type & NTS_MSG_BIND)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->retval = src_msg->retval;
		target_msg->nt_errno = src_msg->nt_errno;
	}
	else if (target_msg->msg_type & NTS_MSG_LISTEN)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->retval = src_msg->retval;
	}
	else if (target_msg->msg_type & NTS_MSG_ACCEPT)
	{
		target_msg->sockid = src_msg->sockid;
		memcpy(target_msg->address, src_msg->address, src_msg->addrlen);
		target_msg->addrlen = src_msg->addrlen;
		target_msg->port = src_msg->port;
	}
	else if (target_msg->msg_type & NTS_MSG_DISPATCHED)
	{
		target_msg->retval = src_msg->retval;
		target_msg->sockid = src_msg->sockid;
		target_msg->nt_errno = src_msg->nt_errno;
		target_msg->port = src_msg->port;
	}
	else if (target_msg->msg_type & NTS_MSG_CONNECT)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->conn_status = src_msg->conn_status;
		target_msg->retval = src_msg->retval;
		target_msg->nt_errno = src_msg->nt_errno;
	}
	else if (target_msg->msg_type & NTS_MSG_CLIENT_SYN_ACK)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->retval = src_msg->retval;
	}
	else if (target_msg->msg_type & NTS_MSG_ESTABLISH)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->retval = src_msg->retval;
		target_msg->sockid = src_msg->sockid;
		target_msg->msg_type = src_msg->msg_type;
	}
	else if (target_msg->msg_type & NTS_MSG_CLOSE)
	{
		target_msg->sockid = src_msg->sockid;
	}
	else if (target_msg->msg_type & NTS_MSG_SHUTDOWN)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->retval = src_msg->retval;
	}
	else if (target_msg->msg_type & NTS_MSG_DISCONNECT)
	{
		target_msg->sockid = src_msg->sockid;
	}
	else if (target_msg->msg_type & NTS_MSG_ERR)
	{
		target_msg->sockid = src_msg->sockid;
		target_msg->nt_errno = src_msg->nt_errno;
		target_msg->retval = src_msg->retval;
	}
}
