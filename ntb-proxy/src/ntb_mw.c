/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <rte_io.h>
#include <rte_eal.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_rawdev.h>
#include <rte_rawdev_pmd.h>
#include <rte_memcpy.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_debug.h>
#include <rte_dev.h>
#include <rte_kvargs.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_lcore.h>
#include <rte_bus.h>
// #include <rte_bus_vdev.h>
#include <rte_memzone.h>
#include <rte_mempool.h>
#include <rte_rwlock.h>
#include <errno.h>

#include <time.h>
#include <sched.h>
#include <unistd.h>

#include "ntb_mw.h"

#include "ntb.h"
#include "ntb_hw_intel.h"
#include "config.h"
#include "utils.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

int trans_data_link_cur_index(struct ntb_data_link *data_link)
{
    DEBUG("[trans_data_link_cur_index]\
 recv sync read index request [cur_idx=%ld]\n",
          data_link->local_ring->cur_index);

    *data_link->remote_cum_ptr = data_link->local_ring->cur_index;

    DEBUG("trans cur index to peer, = %ld",
          data_link->local_ring->cur_index);

    return 0;
}

int trans_ctrl_link_cur_index(struct ntb_link_custom *ntb_link)
{
    *ntb_link->ctrl_link->remote_cum_ptr =
        ntb_link->ctrl_link->local_ring->cur_index;

    return 0;
}

int ntb_data_msg_add_header(
    struct ntpacket *msg, uint16_t src_port,
    uint16_t dst_port, int payload_len, int msg_type)
{
    msg->header.src_port = src_port;
    msg->header.dst_port = dst_port;
    msg->header.msg_len = NTPACKET_HEADER_LEN + payload_len;

    // End of Memnode
    if (msg_type == SINGLE_PKG)
    {
        // 001 == single packet: only one packet to send one msg
        msg->header.msg_type |= (1 << 12);
    }
    else if (msg_type == MULTI_PKG)
    {
        // 010 == multi packets: need multi-packets to send one msg
        msg->header.msg_type |= (1 << 13);
    }
    else if (msg_type == ENF_MULTI)
    {
        // 011 == ENF_MULTI: indicate the end/tail packet for one multi-packets msg
        msg->header.msg_type |= (3 << 12);
    }
    else if (msg_type == DETECT_PKG)
    {
        // 000 == DETECT_PKG: sync local (send buffer) read_index
        //  with remote (recv buffer) read_index,
        //	request the read_index of recv buffer on the peer node(ntb endpoint),
        // DETECT_PKG = 000,don't need to do bit operations
    }
    else if (msg_type == FIN_PKG)
    {
        // 100 == FIN_PKG: indicate the NTB FIN between two ntb endpoints
        msg->header.msg_type |= (1 << 14);
    }
    else
    {
        ERR("Error msg_type");
        return -1;
    }

    return 0;
}

int parser_data_len_get_type(
    struct ntb_data_link *data_link, uint16_t msg_type)
{
    if (UNLIKELY(msg_type & 1 << 15))
    {
        trans_data_link_cur_index(data_link);
    }

    msg_type = (msg_type >> 12);
    msg_type &= 0x07;

    return msg_type;
}

int parser_ctrl_msg_header(
    struct ntb_link_custom *ntb_link, uint16_t msg_len)
{
    if (msg_len & (1 << 15))
    {
        trans_ctrl_link_cur_index(ntb_link);
    }

    return 0;
}

int ntb_ctrl_msg_enqueue(
    struct ntb_link_custom *ntlink, struct ntb_ctrl_msg *msg)
{
    DEBUG("ntb_ctrl_msg_enqueue start");
    struct ntb_ring_buffer *r = ntlink->ctrl_link->remote_ring;

    uint64_t next_index =
        r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;

    // looping send
    int full_cnt = 0;
    int sleep_us = 100, cnt_window = 100;

    while (next_index == *ntlink->ctrl_link->local_cum_ptr)
    {
        INFO("ntb_ctrl_msg_enqueue looping");
        full_cnt++;

        if (full_cnt % cnt_window == 0)
        {
            full_cnt = 0;
            usleep(BLOCKING_SLEEP_US);
        }

        sched_yield();
    }

    if ((next_index - *ntlink->ctrl_link->local_cum_ptr) & 0x3ff == 0)
    {
        // PSH = 1
        msg->header.msg_len |= (1 << 15);
    }

    // ptr = r->start_addr + r->cur_index * CTRL_NTPACKET_SIZE, CTRL_NTPACKET_SIZE = 16
    uint8_t *ptr = r->start_addr + (r->cur_index << 4);
    rte_memcpy(ptr, msg, CTRL_NTPACKET_SIZE);
    DEBUG("ntb_ctrl_msg_enqueue end");
    r->cur_index = next_index;

    return 0;
}

int ntb_pure_data_msg_enqueue(
    struct ntb_data_link *data_link, uint8_t *msg, int data_len)
{
    struct ntb_ring_buffer *r = data_link->remote_ring;
    uint64_t next_index =
        r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;

    // looping send
    int full_cnt = 0;
    int sleep_us = 100, cnt_window = 100;

    while (UNLIKELY(next_index == *data_link->local_cum_ptr))
    {
        full_cnt++;
        if (full_cnt % cnt_window == 0)
        {
            full_cnt = 0;
            INFO("[ntb_pure_data_msg_enqueue] \
**blocking 'next_write_idx=%ld, shadown_read_idx=%ld'**\n",
                 next_index, *data_link->local_cum_ptr);
            usleep(BLOCKING_SLEEP_US);
        }
    }

    uint8_t *ptr = r->start_addr +
                   (r->cur_index << NTP_CONFIG.ntb_packetbits_size);
    rte_memcpy(ptr, msg, data_len);

    r->cur_index = next_index;

    return 0;
}

int ntb_data_msg_enqueue2(
    struct ntb_data_link *data_link,
    ntp_msg *outgoing_msg,
    uint16_t src_port, uint16_t dst_port,
    uint16_t payload_len, int msg_type)
{

    // pack the ntpacket header
    uint16_t msg_len = NTPACKET_HEADER_LEN + payload_len;

    if (msg_type == SINGLE_PKG)
    {
        // 001 == single packet: only one packet to send one msg
        outgoing_msg->header.msg_type |= (1 << 12);
    }
    else if (msg_type == MULTI_PKG)
    {
        // 010 == multi packets: need multi-packets to send one msg
        outgoing_msg->header.msg_type |= (1 << 13);
    }
    else if (msg_type == ENF_MULTI)
    {
        // 011 == ENF_MULTI: indicate the end/tail packet for one multi-packets msg
        outgoing_msg->header.msg_type |= (3 << 12);
    }
    else if (msg_type == DETECT_PKG)
    {
        // 000 == DETECT_PKG: sync local (send buffer)
        //  read_index with remote (recv buffer) read_index
        //	request the read_index of recv buffer on the peer node(ntb endpoint)
        // DETECT_PKG = 000,don't need to do bit operations
    }
    else if (msg_type == FIN_PKG)
    {
        // 100 == FIN_PKG: indicate the NTB FIN between two ntb endpoints
        outgoing_msg->header.msg_type |= (1 << 14);
    }
    else
    {
        ERR("Error msg_type");
        return -1;
    }

    // remote write the peer data ringbuffer with write_index
    struct ntb_ring_buffer *peer_dataring = data_link->remote_ring;

    uint64_t next_write_idx;
    next_write_idx =
        peer_dataring->cur_index + 1 <
                peer_dataring->capacity
            ? peer_dataring->cur_index + 1
            : 0;

    int full_cnt = 0;
    int sleep_us = 100, cnt_window = 100;

    while (next_write_idx == *data_link->local_cum_ptr)
    {
        full_cnt++;
        if (full_cnt % cnt_window == 0)
        {
            full_cnt = 0;
            DEBUG("[ntb_data_msg_enqueue2] ** blocking 'next_write_idx=%ld, shadown_read_idx=%ld'**\n",
                  next_write_idx, *data_link->local_cum_ptr);
            usleep(BLOCKING_SLEEP_US);
        }
    }

    uint8_t *write_addr = peer_dataring->start_addr +
                          (peer_dataring->cur_index << NTP_CONFIG.ntb_packetbits_size);

    rte_memcpy(write_addr, &src_port, SRCPORT_SIZE);                  // src_port: 2 bytes
    rte_memcpy(write_addr + DSTPORT_OFFSET, &dst_port, DSTPORT_SIZE); // dst_port: 2 bytes

    if (msg_len <= NTP_CONFIG.data_packet_size)
    {
        // data_link->local_cum_ptr: local cached read_index,
        // when (next_write_idx - shadow_read_idx) % 1024 == 0,
        // request peer node update local read_idx by remote write
        uint64_t wr_gap;
        wr_gap = (next_write_idx >= *data_link->local_cum_ptr)
                     ? (next_write_idx - *data_link->local_cum_ptr)
                     : (next_write_idx - *data_link->local_cum_ptr + peer_dataring->capacity);

        if ((wr_gap & 0x3ff) == 0)
        {
            // PSH: request to update local shadow read_index using remote write by peer side
            INFO("[<= NTP_CONFIG.data_packet_size]=====\
start request the read index of remote ntb dataring==== [next_write_idx=%ld, shadow_read_index=%ld]\n",
                 next_write_idx, *data_link->local_cum_ptr);

            outgoing_msg->header.msg_type != (1 << 15);
        }

        rte_memcpy(write_addr + NTPACKET_HEADER_LEN,
                   (uint8_t *)outgoing_msg->payload, payload_len);
        rte_memcpy(write_addr + MSGLEN_OFFSET,
                   &msg_len, MSGLEN_SIZE); // msg_len: 2 bytes
    }
    else
    {
        // 0xfc00 == 1111 1100 0000 0000
        if (((next_write_idx +
              (msg_len >> NTP_CONFIG.ntb_packetbits_size)) &
             0xfc00) != (peer_dataring->cur_index & 0xfc00))
        {
            INFO("[> NTP_CONFIG.data_packet_size]=====\
start request the read index of remote ntb dataring==== [next_write_idx=%ld, shadow_read_index=%ld]\n",
                 next_write_idx, *data_link->local_cum_ptr);

            // PSH: request to update local shadow read_index using remote write by peer side
            outgoing_msg->header.msg_type != (1 << 15);
        }

        rte_memcpy(write_addr + NTPACKET_HEADER_LEN,
                   (uint8_t *)outgoing_msg->payload,
                   NTP_CONFIG.data_packet_size - NTPACKET_HEADER_LEN);

        rte_memcpy(write_addr + MSGLEN_OFFSET, &msg_len, MSGLEN_SIZE); // msg_len: 2 bytes
    }

    peer_dataring->cur_index = next_write_idx;

    return 0;
}

int ntb_data_msg_enqueue(
    struct ntb_data_link *data_link, struct ntpacket *msg)
{
    struct ntb_ring_buffer *r = data_link->remote_ring;
    //msg_len 为包含头部的总长度，add_header时已经计算完成
    uint16_t msg_len = msg->header.msg_len;
    uint64_t next_index =
        r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;

    //looping send
    int full_cnt = 0;
    int sleep_us = 100, cnt_window = 100;

    // compare the next write_index to current read_index,
    //  judge whether ntb ringbuffer is full
    while (next_index == *data_link->local_cum_ptr)
    {
        full_cnt++;
        if (full_cnt % cnt_window == 0)
        {
            full_cnt = 0;
            INFO("[ntb_data_msg_enqueue] \
**blocking 'next_write_idx=%ld, shadown_read_idx=%ld'**\n",
                 next_index, *data_link->local_cum_ptr);

            usleep(BLOCKING_SLEEP_US);
        }
    }

    uint8_t *ptr = r->start_addr +
                   (r->cur_index << NTP_CONFIG.ntb_packetbits_size);

    // msg_len contains NTPACKET_HEADER_LEN,
    //  msg_len = NTPACKET_HEADER_LEN + payload_length
    if (msg_len <= NTP_CONFIG.data_packet_size)
    {
        // remote peer write_index, read_index, 0x3ff == 1023
        uint64_t wr_gap;
        wr_gap = (next_index >= *data_link->local_cum_ptr)
                     ? (next_index - *data_link->local_cum_ptr)
                     : (next_index - *data_link->local_cum_ptr + r->capacity);

        if ((wr_gap & 0x3ff) == 0)
        {
            // PSH: update local shadow read_index using remote write by peer side
            msg->header.msg_type |= (1 << 15);
        }

        rte_memcpy(ptr + NTPACKET_HEADER_LEN,
                   msg->payload, msg_len - NTPACKET_HEADER_LEN);
        rte_memcpy(ptr, &msg->header, NTPACKET_HEADER_LEN);
    }
    else
    {
        // 0xfc00 == 1111 1100 0000 0000
        if (UNLIKELY(((next_index +
                       (msg_len >> NTP_CONFIG.ntb_packetbits_size)) &
                      0xfc00) != (r->cur_index & 0xfc00)))
        {
            msg->header.msg_type |= (1 << 15);
        }

        rte_memcpy(ptr + NTPACKET_HEADER_LEN,
                   msg->payload, NTP_CONFIG.data_packet_size - NTPACKET_HEADER_LEN);
        rte_memcpy(ptr, &msg->header, NTPACKET_HEADER_LEN);
    }
    r->cur_index = next_index;

    return 0;
}

/** new added methods for different packet size */
int pack_ntpacket_header(
    struct ntpacket_header *packet_header,
    uint16_t src_port, uint16_t dst_port,
    int payload_len, int msg_type)
{
    packet_header->src_port = src_port;
    packet_header->dst_port = dst_port;
    packet_header->msg_len = NTPACKET_HEADER_LEN + payload_len;
    packet_header->msg_type = 0;

    if (msg_type == SINGLE_PKG)
    {
        // 001 == single packet: only one packet to send one msg
        packet_header->msg_type |= (1 << 12);
    }
    else if (msg_type == MULTI_PKG)
    {
        // 010 == multi packets: need multi-packets to send one msg
        packet_header->msg_type |= (1 << 13);
    }
    else if (msg_type == ENF_MULTI)
    {
        // 011 == ENF_MULTI: indicate the end/tail packet for one multi-packets msg
        packet_header->msg_type |= (3 << 12);
    }
    else if (msg_type == DETECT_PKG)
    {
        // 000 == DETECT_PKG: sync local (send buffer)
        //  read_index with remote (recv buffer) read_index
        //	request the read_index of recv buffer on the peer node(ntb endpoint)
        // DETECT_PKG = 000,don't need to do bit operations
    }
    else if (msg_type == FIN_PKG)
    {
        // 100 == FIN_PKG: indicate the NTB FIN between two ntb endpoints
        packet_header->msg_type |= (1 << 14);
    }
    else
    {
        ERR("Error msg_type");
        return -1;
    }

    return 0;
}

int ntpacket_enqueue(
    struct ntb_data_link *data_link,
    struct ntpacket_header *packet_header, ntp_msg *source_msg)
{
    assert(data_link);
    assert(packet_header);
    assert(source_msg);

    struct ntb_ring_buffer *peer_dataring = data_link->remote_ring;
    uint16_t msg_len = packet_header->msg_len;

    DEBUG("[ntpacket_enqueue] msg_len=%d, packet_header->msg_len=%d\n",
          msg_len, packet_header->msg_len);

    uint64_t next_index;
    next_index =
        peer_dataring->cur_index + 1 < peer_dataring->capacity
            ? peer_dataring->cur_index + 1 : 0;

    int full_cnt = 0;
    int sleep_us = 100, cnt_window = 50;

    // compare the next write_index to current read_index,
    //  judge whether ntb ringbuffer is full
    while (UNLIKELY(next_index == *data_link->local_cum_ptr))
    {
        full_cnt++;
        if (full_cnt % cnt_window == 0)
        {
            usleep(sleep_us);
        }
    }

    uint8_t *ptr = peer_dataring->start_addr +
                   (peer_dataring->cur_index << NTP_CONFIG.ntb_packetbits_size);

    if (msg_len <= NTP_CONFIG.data_packet_size)
    {
        // remote peer write_index, read_index, 0x3ff == 1023
        if (UNLIKELY(((next_index - *data_link->local_cum_ptr) & 0x3ff) == 0))
        {
            //PSH: update local shadow read_index using remote write by peer side
            packet_header->msg_type |= (1 << 15);
        }
       
        rte_memcpy(ptr, &source_msg->header, NTPACKET_HEADER_LEN);
        rte_memcpy(ptr + NTPACKET_HEADER_LEN,
                   source_msg->payload, msg_len - NTPACKET_HEADER_LEN);
    }
    else
    {
        // 0xfc00 == 1111 1100 0000 0000
        if (UNLIKELY(((next_index +
                       (msg_len >> NTP_CONFIG.ntb_packetbits_size)) &
                      0xfc00) != (peer_dataring->cur_index & 0xfc00)))
        {
            packet_header->msg_type |= (1 << 15);
        }
    
        rte_memcpy(ptr + NTPACKET_HEADER_LEN,
                   source_msg->payload,
                   NTP_CONFIG.data_packet_size - NTPACKET_HEADER_LEN);
        rte_memcpy(ptr, &source_msg->header, NTPACKET_HEADER_LEN);
    }

    peer_dataring->cur_index = next_index;

    return 0;
}

static struct ntb_ring_buffer *
ntb_ring_create(uint8_t *ptr, uint64_t ring_size, uint64_t msg_len)
{
    struct ntb_ring_buffer *r = malloc(sizeof(*r));
    if (UNLIKELY(!r))
    {
        return NULL;
    }

    r->cur_index = 0;
    r->start_addr = ptr;
    r->end_addr = r->start_addr + ring_size;
    r->capacity = ring_size / msg_len;
    DEBUG("ring_capacity == %ld.", r->capacity);

    return r;
}

// init the ntb memory buffer for all ntb_partitions
static int
ntb_mem_formatting(
    struct ntb_link_custom *ntb_link,
    uint8_t *local_ptr, uint8_t *remote_ptr)
{
    uint8_t *data_local_ptr, *data_remote_ptr;

    ntb_link->ctrl_link->local_cum_ptr = (uint64_t *)local_ptr;
    ntb_link->ctrl_link->remote_cum_ptr = (uint64_t *)remote_ptr;
    *ntb_link->ctrl_link->local_cum_ptr = 0;

    ntb_link->ctrl_link->local_ring =
        ntb_ring_create(((uint8_t *)local_ptr + 8),
                        NTP_CONFIG.ctrl_ringbuffer_size, CTRL_NTPACKET_SIZE);
    ntb_link->ctrl_link->remote_ring =
        ntb_ring_create(((uint8_t *)remote_ptr + 8),
                        NTP_CONFIG.ctrl_ringbuffer_size, CTRL_NTPACKET_SIZE);

    data_local_ptr = local_ptr + NTP_CONFIG.ctrl_ringbuffer_size + 8;
    data_remote_ptr = remote_ptr + NTP_CONFIG.ctrl_ringbuffer_size + 8;

    for (int i = 0; i < ntb_link->num_partition; i++)
    {
        ntb_link->partitions[i].data_link->local_cum_ptr = (uint64_t *)data_local_ptr;
        ntb_link->partitions[i].data_link->remote_cum_ptr = (uint64_t *)data_remote_ptr;
        *(ntb_link->partitions[i].data_link->local_cum_ptr) = 0;

        ntb_link->partitions[i].data_link->local_ring =
            ntb_ring_create((uint8_t *)data_local_ptr + 8,
                            NTP_CONFIG.data_ringbuffer_size,
                            1 << NTP_CONFIG.ntb_packetbits_size);
        ntb_link->partitions[i].data_link->remote_ring =
            ntb_ring_create((uint8_t *)data_remote_ptr + 8,
                            NTP_CONFIG.data_ringbuffer_size,
                            1 << NTP_CONFIG.ntb_packetbits_size);

        data_local_ptr = data_local_ptr + NTP_CONFIG.data_ringbuffer_size + 8;
        data_remote_ptr = data_remote_ptr + NTP_CONFIG.data_ringbuffer_size + 8;
    }

    DEBUG("ntb_mem_formatting pass");
    return 0;
}

struct ntb_link_custom *ntb_start(uint16_t dev_id)
{
    int retval;
    retval = rte_rawdev_start(dev_id);
    if (retval != 0)
    {
        return NULL;
    }

    struct ntb_link_custom *ntb_link = malloc(sizeof(*ntb_link));
    if (UNLIKELY(!ntb_link))
    {
        return NULL;
    }

    struct rte_rawdev *dev;
    dev = &rte_rawdevs[dev_id];

    ntb_link->dev_id = dev_id;
    ntb_link->dev = dev;
    ntb_link->hw = dev->dev_private;

    ntb_link->ctrl_link = malloc(sizeof(struct ntb_ctrl_link));
    if (UNLIKELY(!ntb_link->ctrl_link))
    {
        free(ntb_link);
        return NULL;
    }

    // init the ntb_link partitions
    ntb_link->num_partition = NTP_CONFIG.num_partition;
    ntb_link->partitions = (struct ntb_partition *)
        calloc(ntb_link->num_partition, sizeof(struct ntb_partition));
    ntb_link->round_robin_idx = 0;

    DEBUG("Init %d ntb_partition", ntb_link->num_partition);
    for (int i = 0; i < ntb_link->num_partition; i++)
    {
        ntb_link->partitions[i].id = i;
        ntb_link->partitions[i].num_conns = 0;
        ntb_link->partitions[i].recv_packet_counter = 0;
        ntb_link->partitions[i].send_packet_counter = 0;

        ntb_link->partitions[i].data_link =
            (struct ntb_data_link *)malloc(sizeof(struct ntb_data_link));

        // create the list to be send for each ntb_partition, add ring_head/ring_tail
        ntp_send_list_node *send_list_node =
            (ntp_send_list_node *)malloc(sizeof(ntp_send_list_node));

        send_list_node->conn = NULL;
        send_list_node->next_node = send_list_node;
        ntb_link->partitions[i].send_list.ring_head = send_list_node;
        ntb_link->partitions[i].send_list.ring_tail = send_list_node;

        ntb_link->partitions[i].cache_msg_bulks =
            calloc(NTP_CONFIG.bulk_size, sizeof(ntp_msg *));
    }

    DEBUG("ntb dev started, \
NTB buffer formatting for ntb_partition/ctrl_ringbuffer.");

    // get the NTB local&remote memory ptr,then formats them
    uint8_t *local_ptr = (uint8_t *)ntb_link->hw->mz[0]->addr;
    uint8_t *remote_ptr = (uint8_t *)ntb_link->hw->pci_dev->mem_resource[2].addr;
    ntb_mem_formatting(ntb_link, local_ptr, remote_ptr);

    // create the map to find ntb_connection based on port
    ntb_link->port2conn = createHashMap(NULL, NULL);

    ntm_ntp_shm_context_t ntm_ntp = ntm_ntp_shm();
    ntm_ntp_shm_accept(ntm_ntp,
                       NTM_NTP_SHM_NAME, strlen(NTM_NTP_SHM_NAME));
    ntb_link->ntm_ntp = ntm_ntp;

    ntp_ntm_shm_context_t ntp_ntm = ntp_ntm_shm();
    ntp_ntm_shm_accept(ntp_ntm,
                       NTP_NTM_SHM_NAME, strlen(NTP_NTM_SHM_NAME));
    ntb_link->ntp_ntm = ntp_ntm;

    // create the 'ntp_ep_send_queue'
    //  and 'ntp_ep_recv_queue' epoll ring queue.
    int rc;
    epoll_sem_shm_ctx_t ep_send_ctx;
    ep_send_ctx = epoll_sem_shm();
    if (!ep_send_ctx)
    {
        ERR("Failed to allocate memory for 'epoll_sem_shm_ctx_t ep_send_ctx'.");
        goto FAIL;
    }

    rc = epoll_sem_shm_accept(ep_send_ctx, NTP_EP_SEND_QUEUE, strlen(NTP_EP_SEND_QUEUE));
    if (rc != 0)
    {
        ERR("Failed to epoll_sem_shm_accept for 'epoll_sem_shm_ctx_t ep_send_ctx'.");
        goto FAIL;
    }
    ntb_link->ntp_ep_send_ctx = ep_send_ctx;

    epoll_sem_shm_ctx_t ep_recv_ctx;
    ep_recv_ctx = epoll_sem_shm();
    if (!ep_recv_ctx)
    {
        ERR("Failed to allocate memory for 'epoll_sem_shm_ctx_t ep_recv_ctx'.");
        goto FAIL;
    }

    rc = epoll_sem_shm_accept(ep_recv_ctx,
                              NTP_EP_RECV_QUEUE, strlen(NTP_EP_RECV_QUEUE));
    if (rc != 0)
    {
        ERR("Failed to epoll_sem_shm_accept for 'epoll_sem_shm_ctx_t ep_recv_ctx'.");
        goto FAIL;
    }
    ntb_link->ntp_ep_recv_ctx = ep_recv_ctx;

    // create the hashmap for caching epoll_context
    ntb_link->epoll_ctx_map = createHashMap(NULL, NULL);

    ntb_link->is_stop = false;

    return ntb_link;

FAIL:
    if (!ntb_link)
    {
        return NULL;
    }

    if (ntb_link->epoll_ctx_map)
    {
        free(ntb_link->epoll_ctx_map);
        ntb_link->epoll_ctx_map = NULL;
    }

    if (ep_recv_ctx)
    {
        epoll_sem_shm_master_close(ep_recv_ctx);
        epoll_sem_shm_destroy(ep_recv_ctx);
    }

    if (ep_send_ctx)
    {
        epoll_sem_shm_master_close(ep_send_ctx);
        epoll_sem_shm_destroy(ep_send_ctx);
    }

    // free resources used between ntp and ntm.
    if (ntb_link->ntp_ntm)
    {
        ntp_ntm_shm_close(ntb_link->ntp_ntm);
        ntp_ntm_shm_destroy(ntb_link->ntp_ntm);
    }

    if (ntb_link->ntm_ntp)
    {
        ntm_ntp_shm_close(ntb_link->ntm_ntp);
        ntm_ntp_shm_destroy(ntb_link->ntm_ntp);
    }

    if (ntb_link->port2conn)
    {
        free(ntb_link->port2conn);
        ntb_link->port2conn = NULL;
    }

    if (ntb_link->ctrl_link)
    {
        if (ntb_link->ctrl_link->local_ring)
        {
            free(ntb_link->ctrl_link->local_ring);
            ntb_link->ctrl_link->local_ring = NULL;
        }

        if (ntb_link->ctrl_link->remote_ring)
        {
            free(ntb_link->ctrl_link->remote_ring);
            ntb_link->ctrl_link->remote_ring = NULL;
        }
    }

    if (ntb_link->num_partition > 0 && ntb_link->partitions)
    {
        for (int i = 0; i < ntb_link->num_partition; i++)
        {
            if (ntb_link->partitions[i].data_link)
            {
                if (ntb_link->partitions[i].data_link->local_ring)
                {
                    free(ntb_link->partitions[i].data_link->local_ring);
                    ntb_link->partitions[i].data_link->local_ring = NULL;
                }

                if (ntb_link->partitions[i].data_link->remote_ring)
                {
                    free(ntb_link->partitions[i].data_link->remote_ring);
                    ntb_link->partitions[i].data_link->remote_ring = NULL;
                }

                free(ntb_link->partitions[i].data_link);
                ntb_link->partitions[i].data_link = NULL;
            }

            if (ntb_link->partitions[i].send_list.ring_head)
            {
                ntp_send_list_node *pre_node =
                    ntb_link->partitions[i].send_list.ring_head;
                ntp_send_list_node *curr_node = pre_node->next_node;
                while (curr_node && curr_node !=
                                        ntb_link->partitions[i].send_list.ring_head)
                {
                    ntp_send_list_node *tmp_node;
                    tmp_node = curr_node;
                    curr_node = curr_node->next_node;

                    free(tmp_node);
                    tmp_node = NULL;
                }

                free(ntb_link->partitions[i].send_list.ring_head);
                ntb_link->partitions[i].send_list.ring_head = NULL;
                ntb_link->partitions[i].send_list.ring_tail = NULL;
            }

            if (ntb_link->partitions[i].cache_msg_bulks)
            {
                free(ntb_link->partitions[i].cache_msg_bulks);
                ntb_link->partitions[i].cache_msg_bulks = NULL;
            }
        }

        free(ntb_link->partitions);
        ntb_link->partitions = NULL;
    }

    if (ntb_link->dev)
    {
        /* Stop traffic and Close port. */
        rte_rawdev_stop(ntb_link->dev_id);
        rte_rawdev_close(ntb_link->dev_id);
    }

    free(ntb_link);
    ntb_link = NULL;

    return NULL;
}

void ntb_destroy(struct ntb_link_custom *ntb_link,
                 struct ntp_lcore_conf *lcore_conf)
{
    if (!ntb_link)
        return;

    if (ntb_link->is_stop)
        return;

    printf("\n***************** NTP Exit... *****************\n");

    ntb_link->is_stop = true;

    struct ntp_lcore_conf *conf;
	uint32_t lcore_id;

    /* Stop transmission first. */
    RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
        conf = &lcore_conf[lcore_id];

		if (!conf->is_enabled)
			continue;

		if (conf->stopped)
			continue;

		conf->stopped = 1;
    }
    printf("\nWaiting for lcores to finish...\n");
	rte_eal_mp_wait_lcore();

    // wait or join for ntb_link->ntm_ntp_listener
    pthread_join(ntb_link->ntm_ntp_listener, NULL);

    pthread_join(ntb_link->ntm_ntp_listener, NULL);
    pthread_join(ntb_link->ctrl_recv_thr, NULL);

    // required by epoll daemon thread
    epoll_sem_try_exit(ntb_link->ntp_ep_recv_ctx);
    pthread_join(ntb_link->epoll_listen_thr, NULL);

    // free epoll-related resources
    HashMapIterator iter;
    if (ntb_link->epoll_ctx_map)
    {
        epoll_context_t epoll_ctx;
        iter = createHashMapIterator(ntb_link->epoll_ctx_map);

        while (hasNextHashMapIterator(iter))
        {
            iter = nextHashMapIterator(iter);
            epoll_ctx = (epoll_context_t)iter->entry->value;

            if (!epoll_ctx)
            {
                continue;
            }

            if (epoll_ctx->ep_conn_map) 
            {
                ntb_conn_t tmp_conn;
                HashMapIterator ep_conn_map_iter;
                ep_conn_map_iter = createHashMapIterator(epoll_ctx->ep_conn_map);

                while (hasNextHashMapIterator(ep_conn_map_iter))
                {
                    ep_conn_map_iter = nextHashMapIterator(ep_conn_map_iter);
                    tmp_conn = (ntb_conn_t)ep_conn_map_iter->entry->value;
                    if (!tmp_conn)
                    {
                        continue;
                    }

                    tmp_conn->epoll = NTS_EPOLLNONE;
                    tmp_conn->ep_data = 0;
                    tmp_conn->sockid = 0;
                    tmp_conn->epoll_ctx = NULL;
                }

                freeHashMapIterator(&ep_conn_map_iter);
                Clear(epoll_ctx->ep_conn_map);
                free(epoll_ctx->ep_conn_map);
                epoll_ctx->ep_conn_map = NULL;
            }

            epoll_ctx->ep_io_queue = NULL;
            if (epoll_ctx->ep_io_queue_ctx)
            {
                ep_event_queue_free(epoll_ctx->ep_io_queue_ctx, false);
                epoll_ctx->ep_io_queue_ctx = NULL;
            }

            free(epoll_ctx);
            epoll_ctx = NULL;
        }

        freeHashMapIterator(&iter);
        Clear(ntb_link->epoll_ctx_map);
        free(ntb_link->epoll_ctx_map);
        ntb_link->epoll_ctx_map = NULL;
    }
    

    // free send/receive epoll context resources
    if (ntb_link->ntp_ep_recv_ctx)
    {
        epoll_sem_shm_master_close(ntb_link->ntp_ep_recv_ctx);
        epoll_sem_shm_destroy(ntb_link->ntp_ep_recv_ctx);
    }
    if (ntb_link->ntp_ep_send_ctx)
    {
        epoll_sem_shm_master_close(ntb_link->ntp_ep_send_ctx);
        epoll_sem_shm_destroy(ntb_link->ntp_ep_send_ctx);
    }

    // free resources used between ntp and ntm.
    if (ntb_link->ntp_ntm)
    {
        ntp_ntm_shm_close(ntb_link->ntp_ntm);
        ntp_ntm_shm_destroy(ntb_link->ntp_ntm);
    }

    if (ntb_link->ntm_ntp)
    {
        ntm_ntp_shm_close(ntb_link->ntm_ntp);
        ntm_ntp_shm_destroy(ntb_link->ntm_ntp);
    }

    for (int i = 0; i < ntb_link->num_partition; i++)
    {
        struct ntb_partition *partition;
        partition = &ntb_link->partitions[i];

        // first remove all ntb-conn in this partition
        ntp_send_list_node *tmp_node, *curr_node;
        curr_node = partition->send_list.ring_head;
        while (curr_node->conn)
        {
            tmp_node = curr_node->next_node;
            free(curr_node);
            curr_node = tmp_node;
        }
        if (curr_node)
        {
            free(curr_node);
        }

        // free cache_msg_bulks
        free(partition->cache_msg_bulks);

        // free data_link
        free(partition->data_link);
    }

    // free partitions
    if (ntb_link->partitions)
    {
        free(ntb_link->partitions);
        ntb_link->partitions = NULL;
    }

    // free ctrl_link
    if (ntb_link->ctrl_link)
    {
        free(ntb_link->ctrl_link);
        ntb_link->ctrl_link = NULL;
    }

    if (ntb_link->ctrl_link)
    {
        if (ntb_link->ctrl_link->local_ring)
        {
            free(ntb_link->ctrl_link->local_ring);
            ntb_link->ctrl_link->local_ring = NULL;
        }

        if (ntb_link->ctrl_link->remote_ring)
        {
            free(ntb_link->ctrl_link->remote_ring);
            ntb_link->ctrl_link->remote_ring = NULL;
        }
    }

    if (ntb_link->num_partition > 0 && ntb_link->partitions)
    {
        for (int i = 0; i < ntb_link->num_partition; i++)
        {
            if (ntb_link->partitions[i].data_link)
            {
                if (ntb_link->partitions[i].data_link->local_ring)
                {
                    free(ntb_link->partitions[i].data_link->local_ring);
                    ntb_link->partitions[i].data_link->local_ring = NULL;
                }

                if (ntb_link->partitions[i].data_link->remote_ring)
                {
                    free(ntb_link->partitions[i].data_link->remote_ring);
                    ntb_link->partitions[i].data_link->remote_ring = NULL;
                }

                free(ntb_link->partitions[i].data_link);
                ntb_link->partitions[i].data_link = NULL;
            }

            if (ntb_link->partitions[i].send_list.ring_head)
            {
                ntp_send_list_node *pre_node =
                    ntb_link->partitions[i].send_list.ring_head;
                ntp_send_list_node *curr_node = pre_node->next_node;
                while (curr_node && curr_node !=
                                        ntb_link->partitions[i].send_list.ring_head)
                {
                    ntp_send_list_node *tmp_node;
                    tmp_node = curr_node;
                    curr_node = curr_node->next_node;

                    free(tmp_node);
                    tmp_node = NULL;
                }

                free(ntb_link->partitions[i].send_list.ring_head);
                ntb_link->partitions[i].send_list.ring_head = NULL;
                ntb_link->partitions[i].send_list.ring_tail = NULL;
            }

            if (ntb_link->partitions[i].cache_msg_bulks)
            {
                free(ntb_link->partitions[i].cache_msg_bulks);
                ntb_link->partitions[i].cache_msg_bulks = NULL;
            }
        }

        free(ntb_link->partitions);
        ntb_link->partitions = NULL;
    }

    // free all ntb_connection list
    iter = createHashMapIterator(ntb_link->port2conn);
    while (hasNextHashMapIterator(iter))
    {
        iter = nextHashMapIterator(iter);
        ntb_conn_t conn = (ntb_conn_t)iter->entry->value;

        conn->partition_id = -1;
        conn->partition = NULL;

        if (conn->nts_send_ring)
        {
            ntp_shm_close(conn->nts_send_ring);
            ntp_shm_destroy(conn->nts_send_ring);
        }

        if (conn->nts_recv_ring)
        {
            ntp_shm_close(conn->nts_recv_ring);
            ntp_shm_destroy(conn->nts_recv_ring);
        }

        free(conn);
        conn = NULL;
    }
    freeHashMapIterator(&iter);
    Clear(ntb_link->port2conn);
    free(ntb_link->port2conn);
    ntb_link->port2conn = NULL;

    uint16_t dev_id = ntb_link->dev_id;

    // free ntb_link
    free(ntb_link);
    ntb_link = NULL;

    /* Stop traffic and Close port. */
    printf("\nStop traffic and Close port...\n");
	rte_rawdev_stop(dev_id);
	rte_rawdev_close(dev_id);

    DEBUG("ntb_destroy success");
    printf("\n***************** NTP Exit Completely *****************\n");
}
