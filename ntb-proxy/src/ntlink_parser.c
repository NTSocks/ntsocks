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

#include "ntb_mem.h"
#include "ntm_msg.h"
#include "ntlink_parser.h"
#include "ntb_proxy.h"
#include "ntp_shm.h"

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
    conn->state = 0;
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
    ntp_shm_close(conn->send_ring);
    ntp_shm_close(conn->recv_ring);
    ntp_shm_destroy(conn->send_ring);
    Remove(ntlink->map, conn_name);
    free(conn);
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
    struct ntb_ctrl_msg *msg = malloc(sizeof(*msg));
    uint64_t read_index = get_read_index(ntlink, src_port, dst_port);
    msg->header.src_port = src_port;
    msg->header.dst_port = dst_port;
    msg->header.msg_len = 6;
    rte_memcpy(msg->msg, &read_index, 8);
    ntb_ctrl_msg_enqueue(ntlink, msg);
    return 0;
}

int index_ctrl_handler(struct ntb_link *ntlink, struct ntb_ctrl_msg *msg)
{
    //解析包时将源端口和目的端口调换。
    char *conn_name = create_conn_name(msg->header.dst_port, msg->header.src_port);
    ntb_conn *conn = Get(ntlink->map, conn_name);
    conn->send_ring->ntsring_handle->opposite_read_index = *(uint64_t *)msg->msg;
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

static int ctrl_trans_cum_ptr(struct ntb_link *ntlink)
{
    *ntlink->ctrllink->remote_cum_ptr = ntlink->ctrllink->local_ring->cur_serial;
    return 0;
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
        mss->header.msg_len |= 1 << 15;
    }
    uint8_t *ptr = r->start_addr + r->cur_serial * NTB_CTRL_MSG_TL;
    rte_memcpy(ptr, msg, NTB_CTRL_MSG_TL);
    r->cur_serial = next_serial;
    return 0;
}

int ntb_ctrl_msg_dequeue(struct ntb_link *ntlink)
{
	struct ntb_ring *r = ntlink->ctrllink->local_ring;
	uint8_t *ptr = r->start_addr + (r->cur_serial * NTB_CTRL_MSG_TL);
	struct ntb_ctrl_msg *msg = (struct ntb_ctrl_msg *)ptr;
	int msg_len = msg->header.mss_len;
	if (msg_len == 0)
	{
		return -1;
	}
	int msg_type = ctrl_msg_header_parser(ntlink, msg);
    index_ctrl_handler(ntlink,msg);
	msg->header.msg_len = 0;
	r->cur_serial = (r->cur_serial + 1) % (r->capacity);
	return 0;
}

// static struct ntb_ctrl_msg *creat_ctrl_msg(uint8_t msg_type, uint8_t msg_len, uint16_t src_port, uint16_t dst_port)
// {
//     struct ntb_ctrl_msg *reply_msg = malloc(sizeof(*reply_msg));
//     reply_msg->header.msg_type = msg_type;
//     reply_msg->header.msg_len = msg_len;
//     reply_msg->header.src_port = src_port;
//     reply_msg->header.dst_port = dst_port;
//     return *reply_msg;
// }
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