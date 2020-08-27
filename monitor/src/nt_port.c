/*
 * <p>Title: port.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2020 FDU NiSL</p>
 *
 * @author Wenxiong Zou
 * @date Jan 6, 2020 
 * @version 1.0
 */

#include <stdlib.h>
#include "nt_port.h"
#include "nt_log.h"
#include "utils.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

void init_port_context(nt_port_context_t nt_port_ctx, int max_port)
{
    int i;
    if (max_port <= 0)
    {
        ERR("max port must be positive.");
        return;
    }

    nt_port_ctx->num_ports = max_port;
    nt_port_ctx->ntport = (nt_port_t)calloc(max_port, sizeof(struct nt_port));
    if (!nt_port_ctx->ntport)
    {
        ERR("Failed to allocate memory for nt_port.");
        return;
    }
    TAILQ_INIT(&nt_port_ctx->free_ntport);
    for (i = 0; i < max_port; ++i)
    {
        nt_port_ctx->ntport[i].port_id = i;
        nt_port_ctx->ntport[i].status = NT_PORT_UNUSED;
        TAILQ_INSERT_TAIL(&nt_port_ctx->free_ntport, &nt_port_ctx->ntport[i], free_ntport_link);
    }
}

nt_port_t allocate_port(nt_port_context_t nt_port_ctx, int need_lock)
{
    nt_port_t port = NULL;

    if (need_lock)
        pthread_mutex_lock(&nt_port_ctx->port_lock);

    while (port == NULL)
    {
        port = TAILQ_FIRST(&nt_port_ctx->free_ntport);
        if (!port)
        {
            if (need_lock)
                pthread_mutex_unlock(&nt_port_ctx->port_lock);
            ERR("The concurrent ports are at maximum.\n");
        }

        if (UNLIKELY(port->status != NT_PORT_UNUSED)) {
            TAILQ_REMOVE(&nt_port_ctx->free_ntport, port, free_ntport_link);
            port = NULL;
        } else {
            TAILQ_REMOVE(&nt_port_ctx->free_ntport, port, free_ntport_link);
            port->status = NT_PORT_USED;
        }
        
    }

    if (need_lock)
        pthread_mutex_unlock(&nt_port_ctx->port_lock);

    return port;
}


nt_port_t allocate_specified_port(nt_port_context_t nt_port_ctx, int portid, int need_lock)
{
    if (portid < 0 || portid >= nt_port_ctx->num_ports)
    {
        ERR("The port id does not exist");
        return NULL;
    }

    if (need_lock)
        pthread_mutex_lock(&nt_port_ctx->port_lock);

    nt_port_t port = &nt_port_ctx->ntport[portid];
    if (port->status != NT_PORT_UNUSED)
    {
        port = NULL;
    } else {
        port->status = NT_PORT_USED;
    }

    if (need_lock)
            pthread_mutex_unlock(&nt_port_ctx->port_lock);

    return port;
}


void free_port(nt_port_context_t nt_port_ctx, int portid, int need_lock)
{
    if (portid < 0 || portid >= nt_port_ctx->num_ports)
    {
        ERR("The port id does not exist");
        return;
    }

    nt_port_t port = &nt_port_ctx->ntport[portid];
    if (port->status == NT_PORT_UNUSED)
    {
        return;
    }
    port->status = NT_PORT_UNUSED;

    if (need_lock)
        pthread_mutex_lock(&nt_port_ctx->port_lock);

    TAILQ_INSERT_TAIL(&nt_port_ctx->free_ntport, port, free_ntport_link);
    
    if (need_lock)
        pthread_mutex_unlock(&nt_port_ctx->port_lock);
}

nt_port_t get_port(nt_port_context_t nt_port_ctx, int portid, int max_port)
{
    if (portid < 0 || portid >= nt_port_ctx->num_ports)
    {
        ERR("The port id does not exist");
        return NULL;
    }
    return &nt_port_ctx->ntport[portid];
}

int is_occupied(nt_port_context_t nt_port_ctx, int portid, int max_port)
{
    if (portid < 0 || portid >= nt_port_ctx->num_ports)
    {
        ERR("The port id does not exist");
        return -1;
    }
    nt_port_t port = &nt_port_ctx->ntport[portid];
    if (port->status == NT_PORT_UNUSED)
    {
        return 0;
    }
    
    return -1;
}