/*
 * <p>Title: main.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7
 * @date Nov 12, 2019 
 * @version 1.0
 */
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "ntp_nts_shm.h"
#include "ntp_nts_msg.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

static int write_test(ntp_shm_context_t ring)
{
    ntp_msg send_msg;
    send_msg.msg_type = 1;
    send_msg.msg_len = 14;
    char *write_msg = "write success";
    memcpy(send_msg.msg, write_msg, 14);
    uint64_t counter = 0;
    while (1)
    {
        if (ntp_shm_send(ring, &send_msg) == 0)
        {
            counter++;
            DEBUG("write success,counter = %ld", counter);
        }
    }
    return 0;
}

static int read_test(ntp_shm_context_t ring)
{
    ntp_msg *recv_msg = malloc(sizeof(*recv_msg));
    uint64_t counter = 0;
    uint64_t my_read_index = 0;
    while (1)
    {
        if (ntp_shm_recv(ring, recv_msg) == 0)
        {
            counter++;
            my_read_index = ntp_get_read_index(ring->ntsring_handle);
            DEBUG("read success, counter = %ld,index = %ld",counter,my_read_index);
        }
    }
    return 0;
}

int main()
{
    int ret, i;
    ntp_shm_context_t send_ring = ntp_shm();
    char *send_ring_name = "80-80-s";
    ntp_shm_connect(send_ring, send_ring_name, sizeof(send_ring_name));

    ntp_shm_context_t recv_ring = ntp_shm();
    char *recv_ring_name = "80-80-r";
    ntp_shm_connect(recv_ring, recv_ring_name, sizeof(recv_ring_name));
    write_test(send_ring);
    return 0;
}
