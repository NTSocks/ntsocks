#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "epoll_event_queue.h"

#define EP_SHM_ADDR "ep-io-event-queue"  

int main(int argc, char *argv[]) {
    epoll_event_queue_t ep_queue;
    size_t addrlen;
    size_t capacity;

    addrlen = strlen(EP_SHM_ADDR);
    capacity = 8;

    ep_queue = ep_event_queue_create((char *)EP_SHM_ADDR, addrlen, capacity);
    if(!ep_queue) return -1;

    nts_epoll_event_int event;
    event.sockid = 2;
    event.ev.data = 2;
    event.ev.events = 1;

    int rc;
    rc = ep_event_queue_push(ep_queue, &event);
    if (rc) {
        printf("ep_event_queue_push failed\n");
        ep_event_queue_free(ep_queue, true);
        return -1;
    }

    nts_epoll_event_int resp_event;
    rc = ep_event_queue_pop(ep_queue, &resp_event);
    if (rc) {
        printf("ep_event_queue_pop failed\n");
        ep_event_queue_free(ep_queue, true);
        return -1;
    }
    printf("resp_event: sockid=%d, events=%d\n", resp_event.sockid, resp_event.ev.events);

    for (int i = 0; i < 7; i++)
    {
        event.sockid = i+1; 
        event.ev.data = i+1;
        event.ev.events = i + 5;
        rc = ep_event_queue_push(ep_queue, &event);
        if (rc) {
            printf("ep_event_queue_push failed\n");
            ep_event_queue_free(ep_queue, true);
            return -1;
        }
    }

    ep_event_queue_free(ep_queue, false);

    
    ep_queue = ep_event_queue_init((char *)EP_SHM_ADDR, addrlen, capacity);
    if(!ep_queue) return -1;

    for (int i = 0; i < 7; i++)
    {
        nts_epoll_event_int tmp_event;
        rc = ep_event_queue_pop(ep_queue, &tmp_event);
        if (rc) {
            printf("ep_event_queue_push failed\n");
            ep_event_queue_free(ep_queue, true);
            return -1;
        }
        printf("tmp_event: sockid=%d, events=%d\n", tmp_event.sockid, tmp_event.ev.events);
    }

    ep_event_queue_free(ep_queue, true);

    printf("sizeof(nts_epoll_event_int): %lu, sizeof(nts_epoll_event): %lu \n", sizeof(nts_epoll_event_int), sizeof(nts_epoll_event));
    printf("sizeof(epoll_event_queue): %lu\n", sizeof(struct _epoll_event_queue));


    return 0;
}
