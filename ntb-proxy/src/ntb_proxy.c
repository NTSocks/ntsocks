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

#include "ntlink_parser.h"
#include "ntb_proxy.h"

struct ntp_rs_ring ntp_ring_lookup(uint16_t src_port,uint16_t dst_port)
{
	return 0;
}

int ntb_set_mw_trans(struct rte_rawdev *dev, const char *mw_name, int mw_idx, uint64_t mw_size)
{
	struct ntb_hw *hw = dev->dev_private;
	const struct rte_memzone *mz;
	int ret = 0;
	if (hw->ntb_ops->mw_set_trans == NULL)
	{
		NTB_LOG(ERR, "Not supported to set mw.");
		return -ENOTSUP;
	}

	mz = rte_memzone_lookup(mw_name);
	if (!mz)
	{
		mz = rte_memzone_reserve_aligned(mw_name, mw_size, dev->socket_id,
										 RTE_MEMZONE_IOVA_CONTIG, hw->mw_size[mw_idx]);
	}
	if (!mz)
	{
		NTB_LOG(ERR, "Cannot allocate aligned memzone.");
		return -EIO;
	}
	hw->mz[mw_idx] = mz;
	ret = (*hw->ntb_ops->mw_set_trans)(dev, mw_idx, mz->iova, mw_size);
	if (ret)
	{
		NTB_LOG(ERR, "Cannot set mw translation.");
		return ret;
	}
	return ret;
}

int ntb_mss_add_header(struct ntb_custom_message *mss, uint16_t process_id, int payload_len, bool eon)
{
	mss->header.process_id = process_id;
	mss->header.mss_type = DATA_TYPE;
	mss->header.mss_len = NTB_HEADER_LEN + payload_len;
	//End of Memnode
	if (eon)
	{
		mss->header.mss_type |= 1 << 6;
	}
	return 0;
}

int ntb_mss_enqueue(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss)
{
	struct ntb_ring *r = sublink->remote_ring;
	int mss_len = mss->header.mss_len;
	uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
	//looping send
	while (next_serial == *sublink->local_cum_ptr)
	{
	}
	if ((next_serial - *sublink->local_cum_ptr) % REV_MSS_COUNTER == 0)
	{
		//PSH
		mss->header.mss_type |= 1 << 7;
	}
	uint8_t *ptr = r->start_addr + r->cur_serial * MAX_NTB_MSS_LEN;
	rte_memcpy(ptr, mss, mss_len);
	r->cur_serial = next_serial;
	return 0;
}

int ntb_mss_dequeue(struct ntb_custom_sublink *sublink, struct ntb_buff *rev_buff)
{
	struct ntb_ring *r = sublink->local_ring;
	uint8_t *ptr = r->start_addr + (r->cur_serial * MAX_NTB_MSS_LEN);
	struct ntb_custom_message *mss = (struct ntb_custom_message *)ptr;
	int mss_len = mss->header.mss_len;
	//printf("process id == %d,mss_type == %d,mss_len == %d\n", mss->header.process_id, mss->header.mss_type, mss->header.mss_len);
	//cur_serial no data
	if (mss_len == 0)
	{
		return -1;
	}
	int mss_type = ntb_prot_header_parser(sublink, mss);
	//ctrl mss,no need copy to rev_buff
	if (mss_type != DATA_TYPE)
	{
		mss->header.mss_len = 0;
		r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		//printf("not date type\n");
		return -1;
	}
	//rev_buff is NULL,return -1
	if (rev_buff == NULL || rev_buff->buff == NULL)
	{
		NTB_LOG(ERR, "ntb socket rev_buff is NULL.");
		return -1;
	}
	rte_memcpy(rev_buff->buff, mss->mss, mss->header.mss_len - NTB_HEADER_LEN);
	//after dequeue,set the mss_len = 0
	mss->header.mss_len = 0;
	r->cur_serial = (r->cur_serial + 1) % (r->capacity);
	return 0;
}

// int ntb_send(struct ntb_custom_sublink *sublink, uint16_t process_id)
// {
// 	uint64_t data_len = sublink->process_map[process_id].send_buff->data_len;
// 	uint64_t sent = 0;
// 	struct ntb_custom_message *mss = malloc(sizeof(*mss));
// 	while (data_len - sent > (MAX_NTB_MSS_LEN - NTB_HEADER_LEN))
// 	{
// 		rte_memcpy(mss->mss, sublink->process_map[process_id].send_buff->buff + sent, MAX_NTB_MSS_LEN - NTB_HEADER_LEN);
// 		ntb_mss_add_header(mss, process_id, MAX_NTB_MSS_LEN - NTB_HEADER_LEN, false);
// 		ntb_mss_enqueue(sublink, mss);
// 		sent += MAX_NTB_MSS_LEN - NTB_HEADER_LEN;
// 	}
// 	rte_memcpy(mss->mss, sublink->process_map[process_id].send_buff->buff + sent, data_len - sent);
// 	ntb_mss_add_header(mss, process_id, data_len - sent, true);
// 	ntb_mss_enqueue(sublink, mss);
// 	sent = data_len;
// 	return 0;
// }

int ntb_send(struct ntb_custom_sublink *sublink, void *mp_node)
{
	//(struct ntb_mempool_node_header *)mp_node;
	void *mp_change = mp_node;
	uint16_t ntb_port = *(uint16_t *)mp_change;
	uint16_t data_len = *((uint16_t *)mp_change + 1) + MEM_NODE_HEADER_LEB;
	uint64_t sent = MEM_NODE_HEADER_LEB;
	struct ntb_custom_message *mss = malloc(sizeof(*mss));
	while (data_len - sent > (MAX_NTB_MSS_LEN - NTB_HEADER_LEN))
	{
		rte_memcpy(mss->mss, (uint8_t *)mp_node + sent, MAX_NTB_MSS_LEN - NTB_HEADER_LEN);
		ntb_mss_add_header(mss, ntb_port, MAX_NTB_MSS_LEN - NTB_HEADER_LEN, false);
		ntb_mss_enqueue(sublink, mss);
		sent += MAX_NTB_MSS_LEN - NTB_HEADER_LEN;
	}
	rte_memcpy(mss->mss, (uint8_t *)mp_node + sent, data_len - sent);
	ntb_mss_add_header(mss, ntb_port, data_len - sent, true);
	ntb_mss_enqueue(sublink, mss);
	sent = data_len;
	return 0;
}

int ntb_receive(struct ntb_custom_sublink *sublink, struct rte_mempool *recevie_message_pool)
{
	void *receive_buff = NULL;
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
		//volatile uint8_t *ptr = (volatile uint8_t *)(r->start_addr + (r->cur_serial * MAX_NTB_MSS_LEN));
		struct ntb_custom_message *mss = (struct ntb_custom_message *)(r->start_addr + (r->cur_serial * MAX_NTB_MSS_LEN));
		int mss_len = mss->header.mss_len;
		//if no mss,continue
		if (mss_len == 0)
		{
			continue;
		}
		rte_memcpy((uint8_t *)receive_buff + received, mss->mss, mss_len - NTB_HEADER_LEN);
		r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		ntb_prot_header_parser(sublink, mss);
		received += mss_len - NTB_HEADER_LEN;
		mss->header.mss_len = 0;
		//if detected end of node sign,put into recv_ing
		if (mss->header.mss_type &= 1 << 6)
		{
			char recv_temp[12] = "SEC_2_PRI_x";
			recv_temp[10] = (char)(mss->header.process_id + 48);
			const char *recv_que = recv_temp;
			struct rte_ring *recv_ring = rte_ring_lookup(recv_que);
			while (rte_ring_enqueue(recv_ring, receive_buff) < 0)
			{
				NTB_LOG(ERR, "Failed to enqueue receive_buff to recv_ring.");
			}
			break;
		}
	}
	return 0;
}

struct ntb_ring *
rte_ring_create_custom(uint8_t *ptr, uint64_t ring_size)
{
	struct ntb_ring *r = malloc(sizeof(*r));
	//r->ring = intel_ntb_get_peer_mw_addr(*dev, mw_idx);
	r->cur_serial = 0;
	r->start_addr = ptr;
	r->end_addr = r->start_addr + ring_size;
	r->capacity = ring_size / MAX_NTB_MSS_LEN;
	NTB_LOG(DEBUG, "ring_capacity == %ld.", r->capacity);
	return r;
}

int ntb_creat_all_sublink(struct ntb_custom_link *ntb_link, uint8_t *local_ptr, uint8_t *remote_ptr)
{

	for (int i = 0; i < PRIORITY; i++)
	{
		ntb_link->sublink[i].local_cum_ptr = (uint64_t *)local_ptr;
		ntb_link->sublink[i].remote_cum_ptr = (uint64_t *)remote_ptr;
		*ntb_link->sublink[i].local_cum_ptr = 0;
		ntb_link->sublink[i].ctrl_mss = malloc(sizeof(struct ntb_ctrl_mss));
		ntb_link->sublink[i].ctrl_mss->local_ctrl_mss = (char *)local_ptr + 8;
		ntb_link->sublink[i].ctrl_mss->remote_ctrl_mss = (char *)remote_ptr + 8;
		ntb_link->sublink[i].ctrl_mss->local_send_flag = (uint8_t *)ntb_link->sublink[i].ctrl_mss->local_ctrl_mss + 14;
		ntb_link->sublink[i].ctrl_mss->remote_rev_flag = (uint8_t *)ntb_link->sublink[i].ctrl_mss->remote_ctrl_mss + 15;
		ntb_link->sublink[i].local_ring = rte_ring_create_custom(((uint8_t *)local_ptr + 24), RING_SIZE);
		ntb_link->sublink[i].remote_ring = rte_ring_create_custom(((uint8_t *)remote_ptr + 24), RING_SIZE);
		ntb_link->sublink[i].process_map = malloc(sizeof(struct ntb_socket) * 0x4000000);
		local_ptr = (uint8_t *)local_ptr + RING_SIZE + 24;
		remote_ptr = (uint8_t *)remote_ptr + RING_SIZE + 24;
	}
	return 0;
}

struct ntb_custom_link *
ntb_start(uint16_t dev_id)
{
	struct rte_rawdev *dev;
	NTB_LOG(DEBUG, "Start dev_id=%d.", dev_id);

	dev = &rte_rawdevs[dev_id];

	struct ntb_custom_link *ntb_link = malloc(sizeof(*ntb_link));
	ntb_link->dev = dev;
	ntb_link->hw = dev->dev_private;
	ntb_link->sublink = malloc(sizeof(struct ntb_custom_sublink) * PRIORITY);
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
	ntb_creat_all_sublink(ntb_link, local_ptr, remote_ptr);
	return ntb_link;
}
