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
    char *send_buff = "s" char *recv_buff = "r" char *src_port_str = int_to_char(src_port);
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

static char *create_conn_name(uint16_t src_port, uint16_t dst_port)
{
    char conn_name[14];
    char *bar = "-";
    char *src_port_str = int_to_char(src_port);
    char *dst_port_str = int_to_char(dst_port);
    strcpy(conn_name, src_port);
    strcat(conn_name, bar);
    strcat(conn_name, dst_port_str);
    return conn_name;
}

int ntp_create_ring_handler(struct ntb_link *ntlink, ntm_msg *msg, ntb_conn *conn)
{
    conn->name = create_conn_name(msg->src_port, msg->dst_port);
    nts_shm_context_t send_ring = nts_shm();
    char *send_ring_name = create_ring_name(msg->src_port, msg->dst_port, true);
    if (nts_shm_accept(send_ring, send_ring_name, sizeof(send_ring_name)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
    nts_shm_context_t recv_ring = nts_shm();
    char *recv_ring_name = create_ring_name(msg->src_port, msg->dst_port, false);
    if (nts_shm_accept(recv_ring, recv_ring_name, sizeof(recv_ring_name)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
    add_shmring_to_ntlink(ntlink, send_ring);
    conn->send_ring = send_ring;
    conn->recv_ring = recv_ring;
    Put(ntlink->map, conn->name, conn);
    return 0;
}

int ntp_destory_ring_handler(struct ntb_link *ntlink, ntm_msg *msg)
{
    char *conn_name = create_conn_name(msg->src_port, msg->dst_port);
    ntb_conn *conn = Get(ntlink->map, conn_name);
    nts_shm_close(conn->send_ring);
    nts_shm_close(conn->recv_ring);
    nts_shm_destroy(conn->recv_ring);
    Remove(ntlink->map, conn_name);
    return 0;
}

static int ntb_ctrl_msg_enqueue(struct ntb_link *ntlink, struct ntb_ctrl_message *msg)
{
    struct ntb_ring *r = ntlink->ctrllink;

    uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
    //looping send
    while (next_serial == *ntlink->ctrllink->local_cum_ptr)
    {
    }
    if ((next_serial - *ntlink->ctrllink->local_cum_ptr) & 0x3ff == 0)
    {
        //PSH
        mss->header.mss_type |= 1 << 15;
    }
    uint8_t *ptr = r->start_addr + r->cur_serial * NTB_CTRL_MSG_TL;
    rte_memcpy(ptr, msg, NTB_CTRL_MSG_TL);
    r->cur_serial = next_serial;
    return 0;
}

uint64_t get_read_index(struct ntb_link *ntlink, uint16_t src_port, uint16_t dst_port)
{
    char *conn_name = create_conn_name(src_port, dst_port);
    ntb_conn *conn = Get(ntlink->map, conn_name);
    uint64_t read_index = conn->recv_ring->ntsring_handle->shmring->read_index;
    return read_index;
}

int ntp_trans_read_index(struct ntb_link *ntlink, uint16_t src_port, uint16_t dst_port)
{
    struct ntb_ctrl_message *msg = malloc(sizeof(*msg));
    uint64_t read_index = get_read_index(ntlink, src_port, dst_port);
    msg->header.src_port = src_port;
    msg->header.dst_port = dst_port;
    msg->header.msg_len = 6;
    rte_memcpy(msg->msg, &read_index, 8);
    return 0;
}

// struct ntp_rs_ring ntp_rsring_lookup(uint16_t src_port, uint16_t dst_port)
// {
//     return 0;
// }

// struct ntp_rs_ring ntp_shmring_lookup()
// {
//     return 0;
// }

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