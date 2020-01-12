/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */
#include <stdint.h>
#include <errno.h>

#include <rte_memcpy.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <arpa/inet.h>

#include "ntb_mem.h"
#include "ntm_msg.h"
#include "ntlink_parser.h"
#include "ntp_nts_shm.h"
#include "ntm_msg.h"
#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

static char *int_to_char(uint16_t x)
{
    char *str = (char *)malloc(6);
    sprintf(str, "%d", x);
    return str;
}

static char *create_ring_name(uint16_t src_port, uint16_t dst_port, bool is_send)
{
    char *ring_name = (char *)malloc(14);
    char *bar = "-";
    char *send_buff = "s";
    char *recv_buff = "r";
    char *src_port_str = int_to_char(src_port);
    char *dst_port_str = int_to_char(dst_port);
    strcpy(ring_name, src_port_str);
    strcat(ring_name, bar);
    strcat(ring_name, dst_port_str);
    strcat(ring_name, bar);
    free(src_port_str);
    free(dst_port_str);
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

char *create_conn_name(uint16_t src_port, uint16_t dst_port)
{
    char *conn_name = (char *)malloc(12);;
    char *bar = "-";
    char *src_port_str = int_to_char(src_port);
    char *dst_port_str = int_to_char(dst_port);
    strcpy(conn_name, src_port_str);
    strcat(conn_name, bar);
    strcat(conn_name, dst_port_str);
    free(src_port_str);
    free(dst_port_str);
    return conn_name;
}

static int create_conn_ack(struct ntb_link *ntlink, ntm_ntp_msg *msg)
{
    ntp_ntm_msg reply_msg;
    reply_msg.src_ip = msg->src_ip;
    reply_msg.dst_ip = msg->dst_ip;
    reply_msg.src_port = msg->src_port;
    reply_msg.dst_port = msg->dst_port;
    reply_msg.msg_type = CREATE_CONN_ACK;
    reply_msg.msg_len = 14;
    ntp_ntm_shm_send(ntlink->ntp_ntm, &reply_msg);
    return 0;
}

int ntp_create_conn_handler(struct ntb_link *ntlink, ntm_ntp_msg *msg)
{
    ntb_conn *conn = malloc(sizeof(*conn));
    conn->state = READY_CONN;
    // conn->detect_send = 0;
    conn->name = create_conn_name(msg->src_port, msg->dst_port);
    ntp_shm_context_t send_ring = ntp_shm();
    char *send_ring_name = create_ring_name(msg->src_port, msg->dst_port, true);
    if (ntp_shm_accept(send_ring, send_ring_name, sizeof(send_ring_name)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
    ntp_shm_context_t recv_ring = ntp_shm();
    char *recv_ring_name = create_ring_name(msg->src_port, msg->dst_port, false);
    if (ntp_shm_accept(recv_ring, recv_ring_name, sizeof(recv_ring_name)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
    conn->send_ring = send_ring;
    conn->recv_ring = recv_ring;
    add_conn_to_ntlink(ntlink, conn);
    Put(ntlink->map, conn->name, conn);
    //向ntp_ntm队列中返回确认消息
    create_conn_ack(ntlink, msg);
    free(send_ring_name);
    free(recv_ring_name);
    DEBUG("create conn success,conn name is %s",conn->name);
    return 0;
}

//不需要了
// int ntp_destory_conn_handler(struct ntb_link *ntlink, ntm_msg *msg)
// {
//     //或者控制通道只负责改变conn-state。close和remove均在遍历时进行
//     //destory改变conn-state。遍历send——data发送FIN-PKG。之后close、移除并free conn。返回确认信息
//     //对端接收到FIN—PKG。改变conn-state。遍历send-data同时close。移除并free conn。返回确认信息
//     // char *conn_name = create_conn_name(msg->src_port, msg->dst_port);
//     // ntb_conn *conn = Get(ntlink->map, conn_name);
//     // conn->state = ACTIVE_CLOSE;
//     // return 0;
// }

uint64_t get_read_index(struct ntb_link *ntlink, uint16_t src_port, uint16_t dst_port)
{
    char *conn_name = create_conn_name(src_port, dst_port);
    ntb_conn *conn = (ntb_conn *)Get(ntlink->map, conn_name);
    free(conn_name);
    uint64_t read_index = ntp_get_read_index(conn->recv_ring->ntsring_handle);
    return read_index;
}

static int ntb_ctrl_msg_enqueue(struct ntb_link *ntlink, struct ntb_ctrl_msg *msg)
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
        msg->header.msg_len |= 1 << 15;
    }
    uint8_t *ptr = r->start_addr + (r->cur_serial * NTB_CTRL_MSG_TL);
    rte_memcpy(ptr, msg, NTB_CTRL_MSG_TL);
    r->cur_serial = next_serial;
    return 0;
}

int ntp_trans_read_index(struct ntb_link *ntlink, uint16_t src_port, uint16_t dst_port)
{
    struct ntb_ctrl_msg msg;
    uint64_t read_index = get_read_index(ntlink, src_port, dst_port);

    msg.header.src_port = src_port;
    msg.header.dst_port = dst_port;
    msg.header.msg_len = 6;
    
    rte_memcpy(msg.msg, &read_index, 8);
    ntb_ctrl_msg_enqueue(ntlink, &msg);
    INFO("send my read_index = %ld",read_index);
    return 0;
}

// int index_ctrl_handler(struct ntb_link *ntlink, struct ntb_ctrl_msg *msg)
// {
//     //解析包时将源端口和目的端口调换。
//     char *conn_name = create_conn_name(msg->header.dst_port, msg->header.src_port);
//     ntb_conn *conn = (ntb_conn *)Get(ntlink->map, conn_name);
//     ntp_set_opposide_readindex(conn->send_ring->ntsring_handle,*(uint64_t *)msg->msg);
//     INFO("set my opposide_readindex");
//     free(conn_name);
//     return 0;
// }

int detect_pkg_handler(struct ntb_link *ntlink, struct ntb_data_msg *msg)
{
    //解析包时将源端口和目的端口调换。
    ntp_trans_read_index(ntlink, msg->header.dst_port, msg->header.src_port);
    return 0;
}

static int ctrl_trans_cum_ptr(struct ntb_link *ntlink)
{
    *ntlink->ctrllink->remote_cum_ptr = ntlink->ctrllink->local_ring->cur_serial;
    return 0;
}

int ctrl_msg_header_parser(struct ntb_link *link, struct ntb_ctrl_msg *msg)
{
    uint16_t msg_len = msg->header.msg_len;
    if (msg_len & 1 << 15)
    {
        ctrl_trans_cum_ptr(link);
    }
    // msg_len &= 0x3f;
    return 0;
}

int ctrl_msg_receive(struct ntb_link *ntlink)
{
    struct ntb_ring *r = ntlink->ctrllink->local_ring;
    while (1)
    {
        __asm__("mfence");
        struct ntb_ctrl_msg *msg = (struct ntb_ctrl_msg *)(r->start_addr + (r->cur_serial * NTB_CTRL_MSG_TL));
        uint16_t msg_len = msg->header.msg_len;
        msg_len &= 0x0fff;
        //if no msg,continue
        if (msg_len == 0)
        {
            continue;
        }
        DEBUG("receive a ntb_ctrl_msg");
        ctrl_msg_header_parser(ntlink, msg);
        //解析接收的包时将src和dst port交换
        uint16_t src_port = msg->header.dst_port;
        uint16_t dst_port = msg->header.src_port;
        char *conn_name = create_conn_name(src_port, dst_port);
        ntb_conn *conn = (ntb_conn *)Get(ntlink->map, conn_name);
        free(conn_name);
        INFO("set my opposide_readindex = %ld",*(uint64_t *)msg->msg);
        ntp_set_opposide_readindex(conn->send_ring->ntsring_handle,*(uint64_t *)msg->msg);
        r->cur_serial = (r->cur_serial + 1) % (r->capacity);
        msg->header.msg_len = 0;
    }
    return 0;
}