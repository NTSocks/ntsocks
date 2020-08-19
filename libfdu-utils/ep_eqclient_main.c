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

    ep_queue = ep_event_queue_init((char *)EP_SHM_ADDR, addrlen, capacity);
    if(!ep_queue) return -1;

    for (int i = 0; i < 7; i++)
    {
        nts_epoll_event_int event;
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

    return 0;
}