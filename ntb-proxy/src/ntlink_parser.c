/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */
#include <stdint.h>
#include <errno.h>

#include <rte_io.h>
#include <rte_eal.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_rawdev.h>
#include <rte_rawdev_pmd.h>
#include <rte_memcpy.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

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
#include <rte_ring.h>

#include <arpa/inet.h>

#include "ntm_msg.h"
#include "ntlink_parser.h"
#include "ntb_proxy.h"
#include "nts_shm.h"

static char *int_to_char(uint16_t x)
{
    char str[6];
    sprintf(str, "%d", x);
    return str;
}

static char *create_ring_name(uint16_t src_port, uint16_t dst_port, bool is_send)
{
    char ring_name[14];
    char *bar = "-";
    char *send_buff = "s" char *recv_buff = "r" 
    char *src_port_str = int_to_char(src_port);
    char *dst_port_str = int_to_char(dst_port);
    strcpy(ring_name, src_port);
    strcat(ring_name, bar);
    strcat(ring_name, dst_port_str);
    strcat(ring_name, bar);
    if (is_send)
    {
        strcat(ring_name, send_buff);
    }
    else
    {
        strcat(ring_name, recv_buff);
    }
    return ring_name;
}
int ntm_create_ring_handler(struct ntb_link *ntlink, ntm_msg *msg)
{
    // uint32_t src_ip = msg->src_ip;
    // uint32_t dst_ip = msg->dst_ip;
    nts_shm_context_t send_ring = nts_shm();
    char *send_ring_name = create_ring_name(msg->src_port,msg->dst_port,true);
    if (nts_shm_accept(send_ring, send_ring_name, sizeof(send_ring_name)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
    nts_shm_context_t recv_ring = nts_shm();
    char *recv_ring_name = create_ring_name(msg->src_port,msg->dst_port,false);
    if (nts_shm_accept(recv_ring, recv_ring_name, sizeof(recv_ring_name)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
}

struct ntp_rs_ring ntp_rsring_lookup(uint16_t src_port, uint16_t dst_port)
{
    return 0;
}

struct ntp_rs_ring ntp_shmring_lookup()
{
    return 0;
}

static int ntb_trans_cum_ptr(struct ntb_link *ntlink)
{
    *ntlink->ctrllink->remote_cum_ptr = ntlink->ctrllink->local_ring->cur_serial;
    return 0;
}

static int ntb_ctrl_enqueue(struct ntb_link *ntlink, struct ntb_ctrl_msg *msg)
{
    struct ntb_ring *r = ntlink->ctrllink->remote_ring;
    int msg_len = msg->header.msg_len;
    uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
    //looping send
    while (next_serial == *ntlink->ctrllink->local_cum_ptr)
    {
    }
    if ((next_serial - *ntlink->ctrllink->local_cum_ptr) & 0x3ff == 0)
    {
        //PSH
        msg->header.msg_type |= 1 << 7;
    }
    uint8_t *ptr = r->start_addr + r->cur_serial * NTB_CTRL_msg_TL;
    rte_memcpy(ptr, msg, msg_len);
    r->cur_serial = next_serial;
    return 0;
}

static struct ntb_ctrl_msg *creat_ctrl_msg(uint8_t msg_type, uint8_t msg_len, uint16_t src_port, uint16_t dst_port)
{
    struct ntb_ctrl_msg *reply_msg = malloc(sizeof(*reply_msg));
    reply_msg->header.msg_type = msg_type;
    reply_msg->header.msg_len = msg_len;
    reply_msg->header.src_port = src_port;
    reply_msg->header.dst_port = dst_port;
    return *reply_msg;
}

struct ntb_ctrl_msg *ntb_ctrl_dequeue(struct ntb_link *ntlink)
{
    struct ntb_ring *r = ntlink->ctrllink->remote_ring;
    uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
    //looping send
    while (next_serial == *ntlink->ctrllink->local_cum_ptr)
    {
    }
    if ((next_serial - *ntlink->ctrllink->local_cum_ptr) & 0x3ff == 0)
    {
        //PSH
        msg->header.msg_type |= 1 << 7;
    }
    uint8_t *ptr = r->start_addr + r->cur_serial * NTB_CTRL_msg_TL;
    rte_memcpy(ptr, msg, msg_len);
    r->cur_serial = next_serial;
    return 0;
}

int ntb_send_conn_req(struct ntb_link *ntlink, uint16_t src_port, uint16_t dst_port)
{
    struct ntb_ctrl_msg *reply_msg = creat_ctrl_msg(CONN_REQ, NTB_HEADER_LEN, src_port, dst_port);
    ntb_ctrl_enqueue(ntlink, reply_msg);
    return 0;
}

//send msg to nt_monitor's ring
int ntb_conn_req_handler(struct ntb_link *ntlink, struct ntb_ctrl_msg *msg)
{
    ntp_rs_ring *ntp_ring = ntp_shmring_lookup();
    //解析时，源端口目的端口交换
    uint16_t src_port = msg->header.dst_port;
    uint16_t dst_port = msg->header.src_port;
    struct ntp_ntm_msg *reply_msg = malloc(sizeof(*reply_msg));

    ntb_msg_enqueue(ntp_ring, reply_msg);
    return 0;
}

int ntm_ntp_conn_req_handler(struct ntb_link *ntlink, struct ntp_ntm_msg *ntm_msg)
{
    ntp_rs_ring *ntp_ring = ntp_shmring_lookup();
    uint16_t src_port = ntm_msg->header.src_port;
    uint16_t dst_port = ntm_msg->header.dst_port;
    if (ntm_msg->header.msg_type == ALLOW)
    {
        char *send_ring_name = NULL;
        char *recv_ring_name = NULL;
        rte_ring_create(send_ring_name, ring_size, rte_socket_id(), flags);
        rte_ring_create(recv_ring_name, ring_size, rte_socket_id(), flags);
    }

    struct ntp_ntm_msg *reply_msg = malloc(sizeof(*reply_msg));

    ntb_msg_enqueue(sublink, reply_msg);
    return 0;
}

int ntb_conn_req_ack_handler(struct ntb_link *ntlink, uint16_t src_port, uint16_t dst_port)
{
    struct ntb_ctrl_msg *reply_msg = creat_ctrl_msg(CONN_REQ_ACK, NTB_HEADER_LEN, src_port, dst_port);
    ntb_ctrl_enqueue(ntlink, reply_msg);
    return 0;
}

int ntb_send_conn_req_ack(struct ntb_sublink *sublink, uint16_t src_port, uint16_t dst_port)
{
    int process_id = msg->header.process_id;
    // if (sublink->process_map[process_id].occupied)
    // {
    //     return -1;
    // }
    ntb_buff_creat(sublink, process_id);
    return 0;
}

int ntb_open_link_fail_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *msg)
{
    int process_id = msg->header.process_id;
    sublink->process_map[process_id].occupied = false;
    return 0;
}

// int ntb_fin_send_remian(struct ntb_custom_sublink *sublink, uint16_t process_id)
// {
//     uint64_t data_len = sublink->process_map[process_id]->send_buff->data_len;
//     uint64_t sent = 0;
//     struct ntb_custom_message *msg = malloc(sizeof(*msg));
//     int not_sent = data_len - sent;
//     while (not_sent > 0)
//     {
//         if (not_sent > (MAX_NTB_msg_LEN - NTB_HEADER_LEN))
//         {
//             rte_memcpy(msg->msg, sublink->process_map[process_id]->send_buff->buff + sent, MAX_NTB_msg_LEN - NTB_HEADER_LEN);
//             ntb_msg_add_header(msg, process_id, MAX_NTB_msg_LEN - NTB_HEADER_LEN);
//             not_sent -= MAX_NTB_msg_LEN - NTB_HEADER_LEN;
//         }
//         else
//         {
//             rte_memcpy(msg->msg, sublink->process_map[process_id]->send_buff->buff + sent, not_sent);
//             ntb_msg_add_header(msg, process_id, not_sent);
//             not_sent = 0;
//         }
//         ntb_msg_enqueue(sublink, msg);
//     }
//     return 0;
// }

// int ntb_fin_link_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *msg)
// {
//     int process_id = msg->header.process_id;
//     //how to handler rev_buff?
//     free(sublink->process_map[process_id].rev_buff);
//     sublink->process_map[process_id].rev_buff = NULL;

//     ntb_send(sublink, process_id);
//     sublink->process_map[process_id].occupied = false;
//     free(sublink->process_map[process_id].send_buff);
//     sublink->process_map[process_id].send_buff = NULL;
//     return 0;
// }

int ntb_fin_link_ack_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *msg)
{
    int process_id = msg->header.process_id;
    sublink->process_map[process_id].occupied = false;
    free(sublink->process_map[process_id].rev_buff);
    sublink->process_map[process_id].rev_buff = NULL;
    return 0;
}

int ntb_prot_header_parser(struct ntb_sublink *sublink, struct ntb_custom_message *msg)
{
    int msg_type = msg->header.msg_type;
    if (msg_type & 1 << 7)
    {
        ntb_trans_cum_ptr(sublink);
    }
    msg_type &= 0x3f;
    switch (msg_type)
    {
    case DATA_TYPE:
        break;
    case OPEN_LINK:
        ntb_open_link_handler(sublink, msg);
        break;
    case OPEN_LINK_ACK:
        ntb_open_link_ack_handler(sublink, msg);
        break;
    case OPEN_LINK_FAIL:
        ntb_open_link_fail_handler(sublink, msg);
        break;
    case FIN_LINK:
        //ntb_fin_link_handler(sublink, msg);
        break;
    case FIN_LINK_ACK:
        ntb_fin_link_ack_handler(sublink, msg);
        break;
    default:
        break;
    }
    return msg_type;
}