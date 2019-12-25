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

#include "ntlink_parser.h"
#include "ntb_proxy.h"

struct ntp_rs_ring ntp_rsring_lookup(uint16_t src_port,uint16_t dst_port){
    return 0;
}

struct ntp_rs_ring ntp_shmring_lookup(){
    return 0;
}


static int ntb_trans_cum_ptr(struct ntb_link *ntlink)
{
    *ntlink->ctrllink->remote_cum_ptr = ntlink->ctrllink->local_ring->cur_serial;
    return 0;
}

static int ntb_ctrl_enqueue(struct ntb_link *ntlink, struct ntb_ctrl_mss *mss)
{
	struct ntb_ring *r = ntlink->ctrllink->remote_ring;
	int mss_len = mss->header.mss_len;
	uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
	//looping send
	while (next_serial == *ntlink->ctrllink->local_cum_ptr)
	{
	}
	if ((next_serial - *ntlink->ctrllink->local_cum_ptr) & 0x3ff == 0)
	{
		//PSH
		mss->header.mss_type |= 1 << 7;
	}
	uint8_t *ptr = r->start_addr + r->cur_serial * NTB_CTRL_MSS_TL;
	rte_memcpy(ptr, mss, mss_len);
	r->cur_serial = next_serial;
	return 0;
}

int ntb_send_conn_req(struct ntb_link *ntlink, uint16_t src_port,uint16_t dst_port)
{
    struct ntb_ctrl_mss *reply_mss = malloc(sizeof(*reply_mss));
    reply_mss->header.mss_type = CONN_REQ;
    reply_mss->header.mss_len = NTB_HEADER_LEN;
    reply_mss->header.src_port = src_port;
    reply_mss->header.dst_port = dst_port;
    ntb_ctrl_enqueue(ntlink, reply_mss);
    return 0;
}

int ntb_conn_req_handler(struct ntb_link *ntlink, struct ntb_ctrl_mss *mss)
{
    ntp_rs_ring *ntp_ring = ntp_shmring_lookup();
    //解析时，源端口目的端口交换
    uint16_t src_port = mss->header.dst_port;
    uint16_t dst_port = mss->header.src_port;
    struct ntb_custom_message *reply_mss = malloc(sizeof(*reply_mss));

    if (sublink->process_map[process_id].occupied)
    {
        reply_mss->header.mss_type = OPEN_LINK_FAIL;
        reply_mss->header.mss_len = NTB_HEADER_LEN;
        reply_mss->header.process_id = process_id;
        ntb_mss_enqueue(sublink, reply_mss);
        return -1;
    }
    else
    {
        sublink->process_map[process_id].occupied = true;
        //creat ntb_socket send/rev buffer;
        ntb_buff_creat(sublink, process_id);
        reply_mss->header.mss_type = OPEN_LINK_ACK;
        reply_mss->header.mss_len = NTB_HEADER_LEN;
        reply_mss->header.process_id = process_id;
    }
    ntb_mss_enqueue(sublink, reply_mss);
    return 0;
}

int ntb_send_conn_req_ack(struct ntb_link *ntlink, uint16_t src_port,uint16_t dst_port)
{
    struct ntb_ctrl_mss *reply_mss = malloc(sizeof(*reply_mss));
    reply_mss->header.mss_type = CONN_REQ_ACK;
    reply_mss->header.mss_len = NTB_HEADER_LEN;
    reply_mss->header.src_port = src_port;
    reply_mss->header.dst_port = dst_port;
    ntb_ctrl_enqueue(ntlink, reply_mss);
    return 0;
}



int ntb_open_link_ack_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
{
    int process_id = mss->header.process_id;
    // if (sublink->process_map[process_id].occupied)
    // {
    //     return -1;
    // }
    ntb_buff_creat(sublink, process_id);
    return 0;
}

int ntb_open_link_fail_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
{
    int process_id = mss->header.process_id;
    sublink->process_map[process_id].occupied = false;
    return 0;
}

// int ntb_fin_send_remian(struct ntb_custom_sublink *sublink, uint16_t process_id)
// {
//     uint64_t data_len = sublink->process_map[process_id]->send_buff->data_len;
//     uint64_t sent = 0;
//     struct ntb_custom_message *mss = malloc(sizeof(*mss));
//     int not_sent = data_len - sent;
//     while (not_sent > 0)
//     {
//         if (not_sent > (MAX_NTB_MSS_LEN - NTB_HEADER_LEN))
//         {
//             rte_memcpy(mss->mss, sublink->process_map[process_id]->send_buff->buff + sent, MAX_NTB_MSS_LEN - NTB_HEADER_LEN);
//             ntb_mss_add_header(mss, process_id, MAX_NTB_MSS_LEN - NTB_HEADER_LEN);
//             not_sent -= MAX_NTB_MSS_LEN - NTB_HEADER_LEN;
//         }
//         else
//         {
//             rte_memcpy(mss->mss, sublink->process_map[process_id]->send_buff->buff + sent, not_sent);
//             ntb_mss_add_header(mss, process_id, not_sent);
//             not_sent = 0;
//         }
//         ntb_mss_enqueue(sublink, mss);
//     }
//     return 0;
// }

// int ntb_fin_link_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
// {
//     int process_id = mss->header.process_id;
//     //how to handler rev_buff?
//     free(sublink->process_map[process_id].rev_buff);
//     sublink->process_map[process_id].rev_buff = NULL;

//     ntb_send(sublink, process_id);
//     sublink->process_map[process_id].occupied = false;
//     free(sublink->process_map[process_id].send_buff);
//     sublink->process_map[process_id].send_buff = NULL;
//     return 0;
// }

int ntb_fin_link_ack_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
{
    int process_id = mss->header.process_id;
    sublink->process_map[process_id].occupied = false;
    free(sublink->process_map[process_id].rev_buff);
    sublink->process_map[process_id].rev_buff = NULL;
    return 0;
}

int ntb_prot_header_parser(struct ntb_sublink *sublink, struct ntb_custom_message *mss)
{
    int mss_type = mss->header.mss_type;
    if (mss_type & 1 << 7)
    {
        ntb_trans_cum_ptr(sublink);
    }
    mss_type &= 0x3f;
    switch (mss_type)
    {
    case DATA_TYPE:
        break;
    case OPEN_LINK:
        ntb_open_link_handler(sublink, mss);
        break;
    case OPEN_LINK_ACK:
        ntb_open_link_ack_handler(sublink, mss);
        break;
    case OPEN_LINK_FAIL:
        ntb_open_link_fail_handler(sublink, mss);
        break;
    case FIN_LINK:
        //ntb_fin_link_handler(sublink, mss);
        break;
    case FIN_LINK_ACK:
        ntb_fin_link_ack_handler(sublink, mss);
        break;
    default:
        break;
    }
    return mss_type;
}