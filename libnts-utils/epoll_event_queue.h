#ifndef __EPOLL_EVENT_QUEUE__H__
#define __EPOLL_EVENT_QUEUE__H__

#include <stdint.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>

#include "nts_epoll.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define EPOLL_MAX_ADDRLEN 64
#define DEFAULT_EP_EVENT_QUEUE_SIZE 16

#define EP_SEM_MUTEX_PREFIX "/ep-mutex-"
#define EP_SEM_FULL_PREFIX "/ep-full-"
#define EP_SEM_EMPTY_PREFIX "/ep-empty-"
// like epoll-io-queue-[epoll socket id]
#define EP_SHM_QUEUE_PREFIX "epoll-io-queue-"

        typedef struct _nts_epoll_event_int
        {
                int sockid;
                nts_epoll_event ev;
        } __attribute__((packed)) nts_epoll_event_int;

#define DEFAULT_EP_EVENT_QUEUE_ELE_SIZE sizeof(struct _nts_epoll_event_int)

        typedef struct _nts_event_queue
        {
                volatile uint64_t start; // read_idx
                volatile uint64_t end;   // write_idx
                uint64_t size;           // capacity of event_queue
                uint64_t num_events;     // number of current active events in event_queue
        } nts_event_queue;
        typedef struct _nts_event_queue *nts_event_queue_t;

        typedef struct _epoll_event_queue
        {
                uint64_t shm_fd;
                uint64_t addrlen;
                uint64_t capacity; // the number of total event_queue
                // uint64_t ele_size;       // the size of each element,
                // default sizeof(nts_epoll_event_int)
                uint64_t queue_bytes; // = sizeof(struct _epoll_event_queue)
                                      //      + sizeof(nts_epoll_event_int) * capacity

                char shm_addr[EPOLL_MAX_ADDRLEN];
                char mutex_sem_name[EPOLL_MAX_ADDRLEN];
                char full_sem_name[EPOLL_MAX_ADDRLEN];
                char empty_sem_name[EPOLL_MAX_ADDRLEN];

                sem_t *mutex_sem;
                sem_t *full_sem;
                sem_t *empty_sem;

                nts_event_queue_t shm_queue;
                nts_epoll_event_int *events; // point events array triggerred by
                                             //  listener_socket and client_socket
        } epoll_event_queue;
        typedef struct _epoll_event_queue *epoll_event_queue_t;

        // When master first create shm-based event_queue
        epoll_event_queue_t
        ep_event_queue_create(char *shm_addr, size_t addrlen, size_t capacity);

        // When slave first try to init/connect existing shm-based event_queue
        epoll_event_queue_t
        ep_event_queue_init(char *shm_addr, size_t addrlen, size_t capacity);

        // Returns 0 on success, -1 if buffer is full
        int ep_event_queue_push(
            epoll_event_queue_t ctx, nts_epoll_event_int *event);

        //  Returns 0 on success, -1 if the buffer is empty
        int ep_event_queue_pop(
            epoll_event_queue_t ctx, nts_epoll_event_int *event);

        //  Returns 0 on success, -1 if the buffer is empty
        int ep_event_queue_front(
            epoll_event_queue_t ctx, nts_epoll_event_int *event);

        // Free epoll_event_queue structure, if unlink is 1, unlink all SHM-based variables.
        int ep_event_queue_free(
            epoll_event_queue_t ctx, bool is_unlink);

#ifdef __cplusplus
};
#endif
#endif //!__EPOLL_EVENT_QUEUE__H__