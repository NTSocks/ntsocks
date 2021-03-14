/*
 * <p>Title: epoll_sem_shm.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date July 31, 2020 
 * @version 1.0
 */

#ifndef __EPOLL_SEM_SHM__H__
#define __EPOLL_SEM_SHM__H__

#include "sem_shmring.h"
#include "epoll_msg.h"

#ifdef __cplusplus
extern "C"
{
#endif

        typedef struct _epoll_sem_shm_ctx
        {
                sem_shmring_stat stat;
                sem_shmring_handle_t handle;
                char *shm_addr;
                size_t addrlen;
        } epoll_sem_shm_ctx;
        typedef struct _epoll_sem_shm_ctx *epoll_sem_shm_ctx_t;

        epoll_sem_shm_ctx_t epoll_sem_shm();

        int epoll_sem_shm_accept(
            epoll_sem_shm_ctx_t shm_ctx, char *shm_addr, size_t addrlen);
        int epoll_sem_shm_connect(
            epoll_sem_shm_ctx_t shm_ctx, char *shm_addr, size_t addrlen);

        int epoll_sem_shm_send(
            epoll_sem_shm_ctx_t shm_ctx, epoll_msg *msg);
        int epoll_sem_shm_recv(
            epoll_sem_shm_ctx_t shm_ctx, epoll_msg *recv_msg);

        int epoll_sem_try_exit(epoll_sem_shm_ctx_t shm_ctx);

        int epoll_sem_shm_close(
            epoll_sem_shm_ctx_t shm_ctx, bool is_slave);
        int epoll_sem_shm_master_close(
            epoll_sem_shm_ctx_t shm_ctx);
        int epoll_sem_shm_slave_close(
            epoll_sem_shm_ctx_t shm_ctx);

        void epoll_sem_shm_destroy(
            epoll_sem_shm_ctx_t shm_ctx);

#ifdef __cplusplus
};
#endif

#endif //!__EPOLL_SEM_SHM__H__
