/*
 * <p>Title: nt_backlog.h</p>
 * <p>Description: backlog, hold the established nt_socket
 *      for each ntb-based listener socket.
 *      ntm as producer while nts as consumer.
 *      created by ntm when invoking listen(sockid, backlog).
 *      the owner of backlog is ntm </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Jan 5, 2020 
 * @version 1.0
 */

#ifndef NT_BACKLOG_H_
#define NT_BACKLOG_H_

#include <stdbool.h>
#include <stdio.h>

#include "nt_spsc_shm_queue.h"
#include "socket.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NT_SOCK_SIZE (1 * sizeof(struct nt_backlog_sock))
#define BACKLOG_SIZE 128

    typedef enum nt_backlog_stat
    {
        BACKLOG_UNREADY = 0,
        BACKLOG_READY,
        BACKLOG_CLOSE,
        BACKLOG_UNLINK,
        BACKLOG_ERR
    } nt_backlog_stat;

    struct nt_backlog_context
    {
        nt_backlog_stat backlog_stat;
        nt_spsc_shmring_handle_t shmring_handle;
        char *shm_addr;
        size_t addrlen;

        int backlog_size; // default 128
        nt_socket_t listener_sock;
    };

    typedef struct nt_backlog_context *nt_backlog_context_t;

    /**
     * nt backlog inited by nts listener socket
     * @param listener
     * @param shm_addr
     * @param addrlen
     * @param backlog_size
     * @return
     */
    nt_backlog_context_t backlog_nts(nt_socket_t listener,
                                     char *shm_addr, size_t addrlen, int backlog_size);

    /**
     * nt backlog created by ntm listener socket
     * @param listener
     * @param shm_addr
     * @param addrlen
     * @param backlog_size
     * @return
     */
    nt_backlog_context_t backlog_ntm(nt_socket_t listener,
                                     char *shm_addr, size_t addrlen, int backlog_size);

    /**
     * push an established nt_socket into backlog
     * @param backlog_ctx
     * @param socket
     * @return
     */
    int backlog_push(nt_backlog_context_t backlog_ctx, nt_socket_t socket);

    /**
     * pop an established nt_socket into backlog
     * @param backlog_ctx
     * @param socket
     * @return
     */
    int backlog_pop(nt_backlog_context_t backlog_ctx, nt_socket_t socket);

    /**
     * nts close backlog context
     * @param backlog_ctx
     * @return
     */
    int backlog_nts_close(nt_backlog_context_t backlog_ctx);

    /**
     * ntm close backlog context
     * @param backlog_ctx
     * @return
     */
    int backlog_ntm_close(nt_backlog_context_t backlog_ctx);

    /**
     * destroy the nt backlog context
     * @param backlog_ctx
     */
    void backlog_destroy(nt_backlog_context_t backlog_ctx);

    /**
     * check whether the backlog is full or not
     */
    bool backlog_is_full(nt_backlog_context_t backlog_ctx);

#ifdef __cplusplus
};
#endif

#endif /* NT_BACKLOG_H_ */
