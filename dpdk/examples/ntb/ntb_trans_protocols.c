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
#include <rte_rwlock.h>
#include <rte_ring.h>

#include "ntb.h"
#include "ntb_hw_intel.h"

#include "ntb_trans_protocols.h"

int ntb_buff_creat(struct ntb_custom_sublink *sublink, int process_id)
{
    struct ntb_buff *buff_1 = malloc(sizeof(*buff_1));
    // buff_1->buff = (uint8_t *)malloc(NTB_BUFF_SIZE * sizeof(uint8_t));
    // buff_1->buff_size = NTB_BUFF_SIZE;
    // buff_1->data_len = 0;
    sublink->process_map[process_id].send_buff = buff_1;

    struct ntb_buff *buff_2 = malloc(sizeof(*buff_2));
    // buff_2->buff = (uint8_t *)malloc(NTB_BUFF_SIZE * sizeof(uint8_t));
    // buff_2->buff_size = NTB_BUFF_SIZE;
    // buff_2->data_len = 0;
    sublink->process_map[process_id].rev_buff = buff_2;

    return 0;
}

int ntb_trans_cum_ptr(struct ntb_custom_sublink *sublink)
{
    *sublink->remote_cum_ptr = sublink->local_ring->cur_serial;
    return 0;
}

// int ntb_date_ptr_handler(struct ntb_custom_sublink *sublink)
// {
//     *sublink->remote_cum_ptr = sublink->local_ring->cur_serial;
//     return 0;
// }

int ntb_send_open_link(struct ntb_custom_sublink *sublink, uint16_t process_id)
{
    if (sublink->process_map[process_id].occupied)
    {
        return -1;
    }
    sublink->process_map[process_id].occupied = true;
    struct ntb_custom_message *reply_mss = malloc(sizeof(*reply_mss));
    reply_mss->header.mss_type = OPEN_LINK;
    reply_mss->header.mss_len = NTB_HEADER_LEN;
    reply_mss->header.process_id = process_id;
    ntb_mss_enqueue(sublink, reply_mss);
    return 0;
}

int ntb_send_fin_link(struct ntb_custom_sublink *sublink, uint16_t process_id)
{
    //close send_buff
    free(sublink->process_map[process_id].send_buff);
    sublink->process_map[process_id].send_buff = NULL;
    struct ntb_custom_message *reply_mss = malloc(sizeof(*reply_mss));
    reply_mss->header.mss_type = FIN_LINK;
    reply_mss->header.mss_len = NTB_HEADER_LEN;
    reply_mss->header.process_id = process_id;
    ntb_mss_enqueue(sublink, reply_mss);
    return 0;
}

int ntb_open_link_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
{
    int process_id = mss->header.process_id;
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

int ntb_fin_link_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
{
    int process_id = mss->header.process_id;
    //how to handler rev_buff?
    free(sublink->process_map[process_id].rev_buff);
    sublink->process_map[process_id].rev_buff = NULL;

    ntb_send(sublink, process_id);
    sublink->process_map[process_id].occupied = false;
    free(sublink->process_map[process_id].send_buff);
    sublink->process_map[process_id].send_buff = NULL;
    return 0;
}

int ntb_fin_link_ack_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
{
    int process_id = mss->header.process_id;

    sublink->process_map[process_id].occupied = false;
    free(sublink->process_map[process_id].rev_buff);
    sublink->process_map[process_id].rev_buff = NULL;
    return 0;
}

int ntb_prot_header_parser(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
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
        ntb_fin_link_handler(sublink, mss);
        break;
    case FIN_LINK_ACK:
        ntb_fin_link_ack_handler(sublink, mss);
        break;
    default:
        break;
    }
    return mss_type;
}