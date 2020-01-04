/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */
#include <stdint.h>
#include <errno.h>

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

#include "ntm_shm.h"
#include "ntp_config.h"
#include "ntlink_parser.h"
#include "ntb_mem.h"
#include "ntb_proxy.h"

int ntb_app_mempool_get(struct rte_mempool *mp, void **obj_p, struct ntb_uuid *app_uuid)
{
	if (rte_mempool_get(mp, obj_p) < 0)
	{
		return -1;
	}
	struct ntb_mempool_node_header header = {app_uuid->ntb_port, 0, mp->elt_size};
	rte_memcpy(*obj_p, &header, 6);
	return 0;
}

static int ntb_trans_cum_ptr(struct ntb_sublink *sublink)
{
    *sublink->remote_cum_ptr = sublink->local_ring->cur_serial;
    return 0;
}

int ntb_mem_header_parser(struct ntb_sublink *sublink, struct ntb_data_msg *msg)
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
    default:
        break;
    }
    return msg_type;
}

static int ntb_msg_add_header(struct ntb_data_msg *msg, uint16_t src_port, uint16_t dst_port, int payload_len, bool eon)
{
	msg->header.src_port = src_port;
	msg->header.dst_port = dst_port;
	msg->header.msg_type = DATA_TYPE;
	msg->header.msg_len = NTB_HEADER_LEN + payload_len;
	//End of Memnode
	if (eon)
	{
		msg->header.msg_type |= 1 << 6;
	}
	return 0;
}

static int ntb_msg_enqueue(struct ntb_sublink *sublink, struct ntb_data_msg *msg)
{
	struct ntb_ring *r = sublink->remote_ring;
	int msg_len = msg->header.msg_len;
	uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
	//looping send
	while (next_serial == *sublink->local_cum_ptr)
	{
		
	}
	if ((next_serial - *sublink->local_cum_ptr) & 0x3ff == 0)
	{
		//PSH
		msg->header.msg_type |= 1 << 7;
	}
	uint8_t *ptr = r->start_addr + r->cur_serial * NTB_DATA_MSG_TL;
	rte_memcpy(ptr, msg, msg_len);
	r->cur_serial = next_serial;
	return 0;
}

static int ntb_msg_dequeue(struct ntb_sublink *sublink, uint64_t *ret_msg_len, char *data)
{
	struct ntb_ring *r = sublink->local_ring;
	uint8_t *ptr = r->start_addr + (r->cur_serial * NTB_DATA_MSG_TL);
	struct ntb_data_msg *msg = (struct ntb_data_msg *)ptr;
	int msg_len = msg->header.msg_len;
	//cur_serial no data
	if (msg_len == 0)
	{
		return -1;
	}
	int msg_type = ntb_mem_header_parser(sublink, msg);
	//ctrl msg,no need copy to rev_buff
	if (msg_type != DATA_TYPE)
	{
		msg->header.msg_len = 0;
		r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		return -1;
	}
	*ret_msg_len = msg->header.msg_len - NTB_HEADER_LEN;
	data = msg->msg;
	//after dequeue,set the msg_len = 0
	msg->header.msg_len = 0;
	r->cur_serial = (r->cur_serial + 1) % (r->capacity);
	return 0;
}

int ntb_send_data(struct ntb_sublink *sublink, void *mp_node, uint16_t src_port, uint16_t dst_port)
{
	void *mp_change = mp_node;
	uint16_t ntb_port = *(uint16_t *)mp_change;
	uint16_t data_len = *((uint16_t *)mp_change + 1) + MEM_NODE_HEADER_LEN;
	uint64_t sent = MEM_NODE_HEADER_LEN;

	struct ntb_data_msg *msg = malloc(sizeof(*msg));
	while (data_len - sent > (NTB_DATA_MSG_TL - NTB_HEADER_LEN))
	{
		rte_memcpy(msg->msg, (uint8_t *)mp_node + sent, NTB_DATA_MSG_TL - NTB_HEADER_LEN);
		ntb_msg_add_header(msg, src_port, dst_port, NTB_DATA_MSG_TL - NTB_HEADER_LEN, false);
		ntb_msg_enqueue(sublink, msg);
		sent += NTB_DATA_MSG_TL - NTB_HEADER_LEN;
	}
	rte_memcpy(msg->msg, (uint8_t *)mp_node + sent, data_len - sent);
	ntb_msg_add_header(msg, ntb_port, data_len - sent, true);
	ntb_msg_enqueue(sublink, msg);
	sent = data_len;
	return 0;
}

int ntb_receive(struct ntb_sublink *sublink, struct rte_mempool *recevie_message_pool)
{
	void *receive_buff;
	//get receive node
	while (rte_mempool_get(recevie_message_pool, &receive_buff) < 0)
	{
		NTB_LOG(ERR, "ntb_receive failed to get reveive node.");
	}
	struct ntb_ring *r = sublink->local_ring;
	int received = 0;
	while (1)
	{
		__asm__("mfence");
		struct ntb_data_msg *msg = (struct ntb_data_msg *)(r->start_addr + (r->cur_serial * NTB_DATA_MSG_TL));
		int msg_len = msg->header.msg_len;
		//if no msg,continue
		if (msg_len == 0)
		{
			continue;
		}
		rte_memcpy((uint8_t *)receive_buff + received, msg->msg, msg_len - NTB_HEADER_LEN);
		r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		ntb_mem_header_parser(sublink, msg);
		received += msg_len - NTB_HEADER_LEN;
		msg->header.msg_len = 0;
		//if detected end of node sign,put into recv_ing
		if (msg->header.msg_type &= 1 << 6)
		{
			uint16_t src_port = msg->header.dst_port;
			uint16_t dst_port = msg->header.src_port;
			struct ntp_rs_ring recv_ring = ntp_ring_lookup(src_port, dst_port);
			if (recv_ring == NULL)
			{
				NTB_LOG(ERR, "can't lookup recv_ring.");
				return -1;
			}
			while (rte_ring_enqueue(recv_ring, receive_buff) < 0)
			{
				NTB_LOG(ERR, "Failed to enqueue receive_buff to recv_ring.");
			}
			break;
		}
	}
	return 0;
}

static struct ntb_ring *
ntb_ring_create(uint8_t *ptr, uint64_t ring_size, uint64_t msg_len)
{
	struct ntb_ring *r = malloc(sizeof(*r));
	r->cur_serial = 0;
	r->start_addr = ptr;
	r->end_addr = r->start_addr + ring_size;
	r->capacity = ring_size / msg_len;
	NTB_LOG(DEBUG, "ring_capacity == %ld.", r->capacity);
	return r;
}

static int ntb_mem_formatting(struct ntb_link *ntb_link, uint8_t *local_ptr, uint8_t *remote_ptr)
{
	ntb_link->ctrllink->local_cum_ptr = (uint64_t *)local_ptr;
	ntb_link->ctrllink->remote_cum_ptr = (uint64_t *)remote_ptr;
	ntb_link->ctrllink->local_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), CTRL_RING_SIZE, NTB_CTRL_msg_TL);
	ntb_link->ctrllink->remote_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), CTRL_RING_SIZE, NTB_CTRL_msg_TL);
	local_ptr = (uint8_t *)local_ptr + RING_SIZE + 8;
	remote_ptr = (uint8_t *)remote_ptr + RING_SIZE + 8;
	struct ntp_config *cf = get_ntp_config();
	for (int i = 0; i < cf->nt_proxy.sublink_number; i++)
	{
		ntb_link->sublink[i].local_cum_ptr = (uint64_t *)local_ptr;
		ntb_link->sublink[i].remote_cum_ptr = (uint64_t *)remote_ptr;
		*ntb_link->sublink[i].local_cum_ptr = 0;
		ntb_link->sublink[i].local_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), RING_SIZE, NTB_DATA_MSG_TL);
		ntb_link->sublink[i].remote_ring = ntb_ring_create(((uint8_t *)remote_ptr + 8), RING_SIZE, NTB_DATA_MSG_TL);
		local_ptr = (uint8_t *)local_ptr + RING_SIZE + 8;
		remote_ptr = (uint8_t *)remote_ptr + RING_SIZE + 8;
	}
	return 0;
}

struct ntb_link *
ntb_start(uint16_t dev_id)
{
	struct rte_rawdev *dev;
	NTB_LOG(DEBUG, "Start dev_id=%d.", dev_id);

	dev = &rte_rawdevs[dev_id];

	struct ntp_config *cf = get_ntp_config();

	struct ntb_link *ntb_link = malloc(sizeof(*ntb_link));
	ntb_link->dev = dev;
	ntb_link->hw = dev->dev_private;
	ntb_link->sublink = malloc(sizeof(struct ntb_sublink) * cf->nt_proxy.sublink_number);
	int diag;

	if (dev->started != 0)
	{
		NTB_LOG(ERR, "Device with dev_id=%d already started",
				dev_id);
		return 0;
	}

	diag = (*dev->dev_ops->dev_start)(dev);

	if (diag == 0)
		dev->started = 1;

	//mx_idx = 0 or 1
	uint8_t *local_ptr = (uint8_t *)ntb_link->hw->mz[0]->addr;
	uint8_t *remote_ptr = (uint8_t *)ntb_link->hw->pci_dev->mem_resource[2].addr;
	ntb_mem_formatting(ntb_link, local_ptr, remote_ptr);
	return ntb_link;
}
