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
#include <string.h>

#include "socket.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

void init_socket_context(nt_sock_context_t ntsock_ctx, int max_concurrency)
{
	int i;
	if (max_concurrency <= 0)
	{
		ERR("max concurrency must be positive.");
		return;
	}

	ntsock_ctx->ntsock = (nt_socket_t)calloc(max_concurrency, sizeof(struct nt_socket));
	if (!ntsock_ctx->ntsock)
	{
		ERR("Failed to allocate memory for nt_socket.");
		return;
	}
	TAILQ_INIT(&ntsock_ctx->free_ntsock);
	for (i = 0; i < max_concurrency; ++i)
	{
		ntsock_ctx->ntsock[i].sockid = i;
		ntsock_ctx->ntsock[i].socktype = NT_SOCK_UNUSED;
		memset(&ntsock_ctx->ntsock[i].saddr, 0, sizeof(struct sockaddr_in));
		TAILQ_INSERT_TAIL(&ntsock_ctx->free_ntsock, &ntsock_ctx->ntsock[i], free_ntsock_link);
	}
}

nt_socket_t allocate_socket(nt_sock_context_t ntsock_ctx, int socktype, int need_lock)
{
	nt_socket_t socket = NULL;

	if (need_lock)
		pthread_mutex_lock(&ntsock_ctx->socket_lock);

	while (socket == NULL)
	{
		socket = TAILQ_FIRST(&ntsock_ctx->free_ntsock);
		if (!socket)
		{
			if (need_lock)
				pthread_mutex_unlock(&ntsock_ctx->socket_lock);
			ERR("The concurrent sockets are at maximum.\n");
		}
		TAILQ_REMOVE(&ntsock_ctx->free_ntsock, socket, free_ntsock_link);
	}

	if (need_lock)
		pthread_mutex_unlock(&ntsock_ctx->socket_lock);

	socket->socktype = socktype;
	socket->opts = 0;
	memset(&socket->saddr, 0, sizeof(struct sockaddr_in));
	return socket;
}

void free_socket(nt_sock_context_t ntsock_ctx, int sockid, int need_lock)
{
	nt_socket_t socket = &ntsock_ctx->ntsock[sockid];
	if (socket->socktype == NT_SOCK_UNUSED)
	{
		return;
	}
	socket->socktype = NT_SOCK_UNUSED;

	if (need_lock)
		pthread_mutex_lock(&ntsock_ctx->socket_lock);
	TAILQ_INSERT_TAIL(&ntsock_ctx->free_ntsock, socket, free_ntsock_link);
	if (need_lock)
		pthread_mutex_unlock(&ntsock_ctx->socket_lock);
}

nt_socket_t get_socket(nt_sock_context_t ntsock_ctx, int sockid, int max_concurrency)
{
	if (sockid < 0 || sockid >= max_concurrency)
	{
		return NULL;
	}
	return &ntsock_ctx->ntsock[sockid];
}
