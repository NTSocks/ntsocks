#ifndef __EPOLL_SHM__H__
#define __EPOLL_SHM__H__

#include "shmring.h"
#include "epoll_msg.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define EPOLL_RETRY_TIMES 5

        typedef struct _epoll_shm_context
        {
                shm_stat stat;
                shmring_handle_t handle;
                char *shm_addr;
                size_t addrlen;
        } epoll_shm_context;
        typedef struct _epoll_shm_context *epoll_shm_context_t;

        epoll_shm_context_t epoll_shm();

        int epoll_shm_accept(
            epoll_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);
        int epoll_shm_connect(
            epoll_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

        int epoll_shm_send(
            epoll_shm_context_t shm_ctx, epoll_msg *msg);
        int epoll_shm_recv(
            epoll_shm_context_t shm_ctx, epoll_msg *recv_msg);

        int epoll_shm_close(
            epoll_shm_context_t shm_ctx, bool is_slave);
        int epoll_shm_master_close(
            epoll_shm_context_t shm_ctx);
        int epoll_shm_slave_close(
            epoll_shm_context_t shm_ctx);

        void epoll_shm_destroy(epoll_shm_context_t shm_ctx);

#ifdef __cplusplus
};
#endif

#endif //!__EPOLL_SHM__H__
