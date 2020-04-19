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
#include <rte_bus_vdev.h>
#include <rte_memzone.h>
#include <rte_mempool.h>
#include <rte_rwlock.h>

#include "ntb_mw.h"

#include "ntb.h"
#include "ntb_hw_intel.h"
#include "config.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

int trans_data_link_cur_index(struct ntb_data_link *data_link)
{
    *data_link->remote_cum_ptr = data_link->local_ring->cur_index;
    DEBUG("trans cur index to peer, = %ld",data_link->local_ring->cur_index);
    return 0;
}

int trans_ctrl_link_cur_index(struct ntb_link_custom *ntb_link)
{
    *ntb_link->ctrl_link->remote_cum_ptr = ntb_link->ctrl_link->local_ring->cur_index;
    return 0;
}

int ntb_data_msg_add_header(struct ntb_data_msg *msg, uint16_t src_port, uint16_t dst_port, int payload_len, int msg_type)
{
    msg->header.src_port = src_port;
    msg->header.dst_port = dst_port;
    // msg->header.msg_type = DATA_TYPE;
    msg->header.msg_len = NTB_HEADER_LEN + payload_len;
    //End of Memnode
    if (msg_type == SINGLE_PKG) // 001 == single packet: only one packet to send one msg
    {
        msg->header.msg_len |= (1 << 12);
    }
    else if (msg_type == MULTI_PKG) // 010 == multi packets: need multi-packets to send one msg
    {
        msg->header.msg_len |= (1 << 13);
    }
    else if (msg_type == ENF_MULTI) // 011 == ENF_MULTI: indicate the end/tail packet for one multi-packets msg
    {
        msg->header.msg_len |= (3 << 12);
    }
    else if (msg_type == DETECT_PKG) // 000 == DETECT_PKG: sync local (send buffer) read_index with remote (recv buffer) read_index
    {                                //	request the read_index of recv buffer on the peer node(ntb endpoint)
                                     // DETECT_PKG = 000,don't need to do bit operations
    }
    else if (msg_type == FIN_PKG) // 100 == FIN_PKG: indicate the NTB FIN between two ntb endpoints
    {
        msg->header.msg_len |= (1 << 14);
    }
    else
    {
        ERR("Error msg_type");
    }
    return 0;
}

int parser_data_len_get_type(struct ntb_data_link *data_link, uint16_t msg_len)
{
    DEBUG("parser_data_len_get_type");
    if (msg_len & 1 << 15)
    {
        trans_data_link_cur_index(data_link);
    }
    msg_len = (msg_len >> 12);
    msg_len &= 0x07;
    return msg_len;
}

int parser_ctrl_msg_header(struct ntb_link_custom *ntb_link, uint16_t msg_len)
{
    if (msg_len & (1 << 15))
    {
        trans_ctrl_link_cur_index(ntb_link);
    }
    return 0;
}

int ntb_ctrl_msg_enqueue(struct ntb_link_custom *ntlink, struct ntb_ctrl_msg *msg)
{
    DEBUG("ntb_ctrl_msg_enqueue start");
    struct ntb_ring_buffer *r = ntlink->ctrl_link->remote_ring;

    uint64_t next_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
    //looping send
    while (next_index == *ntlink->ctrl_link->local_cum_ptr)
    {
        INFO("ntb_ctrl_msg_enqueue looping");
    }
    if ((next_index - *ntlink->ctrl_link->local_cum_ptr) & 0x3ff == 0)
    {
        //PSH = 1
        msg->header.msg_len |= (1 << 15);
    }
    //ptr = r->start_addr + r->cur_index * NTB_CTRL_MSG_TL ,NTB_CTRL_MSG_TL = 16
    uint8_t *ptr = r->start_addr + (r->cur_index << 4);
    rte_memcpy(ptr, msg, NTB_CTRL_MSG_TL);
    DEBUG("ntb_ctrl_msg_enqueue end");
    r->cur_index = next_index;
    return 0;
}

int ntb_pure_data_msg_enqueue(struct ntb_data_link *data_link, uint8_t *msg, int data_len)
{
    DEBUG("ntb_pure_data_msg_enqueue start");
    struct ntb_ring_buffer *r = data_link->remote_ring;
    uint64_t next_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
    //looping send
    while (next_index == *data_link->local_cum_ptr)
    {
    }
    //ptr = r->start_addr + r->cur_index * NTB_DATA_MSG_TL ,NTB_DATA_MSG_TL = 128
    uint8_t *ptr = r->start_addr + (r->cur_index << 7);
    rte_memcpy(ptr, msg, data_len);
    DEBUG("ntb_pure_data_msg_enqueue end");
    r->cur_index = next_index;
    return 0;
}

int ntb_data_msg_enqueue(struct ntb_data_link *data_link, struct ntb_data_msg *msg)
{
    DEBUG("ntb_data_msg_enqueue start");
    struct ntb_ring_buffer *r = data_link->remote_ring;
    //msg_len 为包含头部的总长度，add_header时已经计算完成
    uint16_t msg_len = msg->header.msg_len;
    msg_len &= 0x0fff;
    uint64_t next_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
    //looping send
    DEBUG("next_index = %ld,*data_link->local_cum_ptr= %ld",next_index,*data_link->local_cum_ptr);
    while (next_index == *data_link->local_cum_ptr)
    {
        // DEBUG("next_index = %ld,*data_link->local_cum_ptr= %ld",next_index,*data_link->local_cum_ptr);
    }
    uint8_t *ptr = r->start_addr + (r->cur_index << 7);
    if (msg_len <= NTB_DATA_MSG_TL)
    {
        if (((next_index - *data_link->local_cum_ptr) & 0x3ff) == 0)
        {
            //PSH
            msg->header.msg_len |= (1 << 15);
        }
        rte_memcpy(ptr, msg, msg_len);
    }
    else
    {
        if (((next_index + (msg_len >> 7)) & 0xfc00) != (r->cur_index & 0xfc00))
        {
            msg->header.msg_len |= (1 << 15);
        }
        rte_memcpy(ptr, msg, NTB_DATA_MSG_TL);
    }
    DEBUG("enqueue cur_index = %ld",r->cur_index);
    r->cur_index = next_index;
    DEBUG("ntb_data_msg_enqueue len = %d,port1 = %d,port 2 = %d,msg = %s",msg->header.msg_len,msg->header.src_port,msg->header.dst_port,msg->msg);
    DEBUG("ntb_data_msg_enqueue end");
    return 0;
}

static struct ntb_ring_buffer *
ntb_ring_create(uint8_t *ptr, uint64_t ring_size, uint64_t msg_len)
{
    struct ntb_ring_buffer *r = malloc(sizeof(*r));
    r->cur_index = 0;
    r->start_addr = ptr;
    r->end_addr = r->start_addr + ring_size;
    r->capacity = ring_size / msg_len;
    DEBUG("ring_capacity == %ld.", r->capacity);
    return r;
}

static int ntb_mem_formatting(struct ntb_link_custom *ntb_link, uint8_t *local_ptr, uint8_t *remote_ptr)
{
    ntb_link->ctrl_link->local_cum_ptr = (uint64_t *)local_ptr;
    ntb_link->ctrl_link->remote_cum_ptr = (uint64_t *)remote_ptr;
    *ntb_link->ctrl_link->local_cum_ptr = 0;
    ntb_link->ctrl_link->local_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), CTRL_RING_SIZE, NTB_CTRL_MSG_TL);
    ntb_link->ctrl_link->remote_ring = ntb_ring_create(((uint8_t *)remote_ptr + 8), CTRL_RING_SIZE, NTB_CTRL_MSG_TL);
    local_ptr = local_ptr + CTRL_RING_SIZE + 8;
    remote_ptr = remote_ptr + CTRL_RING_SIZE + 8;
    for (int i = 0; i < 1; i++)
    {
        ntb_link->data_link[i].local_cum_ptr = (uint64_t *)local_ptr;
        ntb_link->data_link[i].remote_cum_ptr = (uint64_t *)remote_ptr;
        *ntb_link->data_link[i].local_cum_ptr = 0;
        ntb_link->data_link[i].local_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), DATA_RING_SIZE, NTB_DATA_MSG_TL);
        ntb_link->data_link[i].remote_ring = ntb_ring_create(((uint8_t *)remote_ptr + 8), DATA_RING_SIZE, NTB_DATA_MSG_TL);
        local_ptr = local_ptr + DATA_RING_SIZE + 8;
        remote_ptr = remote_ptr + DATA_RING_SIZE + 8;
    }
    DEBUG("ntb_mem_formatting pass");
    return 0;
}

struct ntb_link_custom *
ntb_start(uint16_t dev_id)
{
    struct rte_rawdev *dev;
    DEBUG("Start dev_id=%d.", dev_id);

    dev = &rte_rawdevs[dev_id];

    load_conf(CONFIG_FILE);

    struct ntb_link_custom *ntb_link = malloc(sizeof(*ntb_link));
    ntb_link->dev = dev;
    ntb_link->hw = dev->dev_private;
    ntb_link->ctrl_link = malloc(sizeof(struct ntb_ctrl_link));
    ntb_link->data_link = malloc(sizeof(struct ntb_data_link) * 1);
    int diag;

    if (dev->started != 0)
    {
        DEBUG("Device with dev_id=%d already started",
              dev_id);
        return 0;
    }

    diag = (*dev->dev_ops->dev_start)(dev);

    if (diag == 0)
        dev->started = 1;
    DEBUG("ntb dev started,mem formatting");

    //get the NTB local&remote memory ptr,then formats them
    uint8_t *local_ptr = (uint8_t *)ntb_link->hw->mz[0]->addr;
    uint8_t *remote_ptr = (uint8_t *)ntb_link->hw->pci_dev->mem_resource[2].addr;
    DEBUG("mem formatting start");
    ntb_mem_formatting(ntb_link, local_ptr, remote_ptr);

    //create the list to be send,add ring_head
    ntp_send_list_node *send_list_node = malloc(sizeof(*send_list_node));
    send_list_node->conn = NULL;
    send_list_node->next_node = send_list_node;
    ntb_link->send_list.ring_head = send_list_node;
    ntb_link->send_list.ring_tail = send_list_node;

    //create the map to find ntb_connection based on port
    ntb_link->port2conn = createHashMap(NULL, NULL);

    ntm_ntp_shm_context_t ntm_ntp = ntm_ntp_shm();
    char *ntm_ntp_name = "ntm-ntp";
    ntm_ntp_shm_accept(ntm_ntp, (char *)ntm_ntp_name, sizeof(ntm_ntp_name));
    ntb_link->ntm_ntp = ntm_ntp;

    ntp_ntm_shm_context_t ntp_ntm = ntp_ntm_shm();
    char *ntp_ntm_name = "ntp-ntm";
    ntp_ntm_shm_accept(ntp_ntm, (char *)ntp_ntm_name, sizeof(ntp_ntm_name));
    ntb_link->ntp_ntm = ntp_ntm;

    return ntb_link;
}
