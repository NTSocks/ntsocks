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
    int rc;

    ep_queue = ep_event_queue_create((char *)EP_SHM_ADDR, addrlen, capacity);
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


    return 0;
}