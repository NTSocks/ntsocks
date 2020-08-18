/*
 * <p>Title: port.h</p>
 * <p>Description: allocate port number for ntb connection</p>
 * <p>Copyright: Copyright (c) 2020 FDU NiSL</p>
 * 
 * usage sample: 
 *      init_port_context(ctx, max_port);
 *      allocate_port(ctx, need_lock);
 *      is_occupied(ctx, portid=0, max_port);
 *      free_port(ctx, portid=0, need_lock);
 *      is_occupied(ctx, portid=0, max_port);
 * 
 * @author Wenxiong Zou
 * @date Jan 6, 2020 
 * @version 1.0
 */

#ifndef NT_PORT_H_
#define NT_PORT_H_

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t nt_port_id;

typedef enum port_status{
    NT_PORT_UNUSED,
    NT_PORT_USED
} port_status;

struct nt_port
{
    nt_port_id port_id;
    int status;
    TAILQ_ENTRY (nt_port) free_ntport_link;
} nt_port;

typedef struct nt_port * nt_port_t;

struct nt_port_context{
    nt_port_t ntport;
    TAILQ_HEAD (, nt_port) free_ntport;
    pthread_mutex_t port_lock;
};

typedef struct nt_port_context * nt_port_context_t;

// init port resources
int init_port_context(nt_port_context_t nt_port_ctx, int max_port);

// allocate one port number to use
nt_port_t allocate_port(nt_port_context_t nt_port_ctx, int need_lock);

// free specified port number
void free_port(nt_port_context_t nt_port_ctx, int portid, int need_lock);

// query a port by portid
nt_port_t get_port(nt_port_context_t nt_port_ctx, int portid, int max_port);

/**
 *  query whether a port number is used
 * 1. If port is unused, return 0;
 * 2. Else if port is used, return 1;
 * 3. Else error, return -1
 */
int is_occupied(nt_port_context_t nt_port_ctx, int portid, int max_port);

#ifdef __cplusplus
};
#endif

#endif /* SOCKET_H_ */