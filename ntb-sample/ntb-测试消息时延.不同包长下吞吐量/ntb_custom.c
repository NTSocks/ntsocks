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


#include "ntb.h"
#include "ntb_hw_intel.h"
#include "ntb_trans_protocols.h"
#include "ntb_custom.h"

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

// int
// ntb_set_mw_trans_custom(struct rte_rawdev *dev,struct rte_memzone *mz,int mw_idx)
// {
// 	struct ntb_hw *hw = dev->dev_private;
// 	int ret = 0;
// 	if (hw->ntb_ops->mw_set_trans == NULL) {
// 		NTB_LOG(ERR, "Not supported to set mw.");
// 		return -ENOTSUP;
// 	}

// 	hw->mz[mw_idx] = mz;
// 	ret = (*hw->ntb_ops->mw_set_trans)(dev, mw_idx, mz->iova, hw->mw_size[mw_idx]);
// 	if (ret) {
// 		NTB_LOG(ERR, "Cannot set mw translation.");
// 		return ret;
// 	}

// 	return ret;
// }
int ntb_mss_add_header(struct ntb_custom_message *mss, uint16_t process_id, int payload_len)
{
	mss->header.process_id = process_id;
	mss->header.mss_type = DATA_TYPE;
	mss->header.mss_len = NTB_HEADER_LEN + payload_len;
	return 0;
}

int ntb_mss_enqueue(struct ntb_custom_sublink *sublink,struct ntb_custom_message *mss)
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
		printf("ntb socket rev_buff is NULL\n");
		return -1;
	}
	rte_memcpy(rev_buff->buff, mss->mss, mss->header.mss_len - NTB_HEADER_LEN);
	//after dequeue,set the mss_len = 0
	mss->header.mss_len = 0;
	r->cur_serial = (r->cur_serial + 1) % (r->capacity);
	return 0;
}

int ntb_send(struct ntb_custom_sublink *sublink, uint16_t process_id)
{
	uint64_t data_len = sublink->process_map[process_id].send_buff->data_len;
	uint64_t sent = 0;
	struct ntb_custom_message *mss = malloc(sizeof(*mss));

	while (data_len - sent > (MAX_NTB_MSS_LEN - NTB_HEADER_LEN))
	{
		rte_memcpy(mss->mss, sublink->process_map[process_id].send_buff->buff + sent, MAX_NTB_MSS_LEN - NTB_HEADER_LEN);
		ntb_mss_add_header(mss, process_id, MAX_NTB_MSS_LEN - NTB_HEADER_LEN);
		ntb_mss_enqueue(sublink, mss);
		sent += MAX_NTB_MSS_LEN - NTB_HEADER_LEN;
	}

	rte_memcpy(mss->mss, sublink->process_map[process_id].send_buff->buff + sent, data_len - sent);
	ntb_mss_add_header(mss, process_id, data_len - sent);
	ntb_mss_enqueue(sublink, mss);
	sent = data_len;
	return 0;
}

// int ntb_send_ring(struct ntb_custom_sublink *sublink, uint16_t process_id,struct rte_ring *send_ring){
	
// }
// int
// ntb_ring_send(struct ntb_ring *r, struct ntb_custom_message *mss)
// {
// 	return 0;
// }

// int
// ntb_ring_receive(struct ntb_ring *r,struct ntb_custom_message *mss)
// {
// 	return 0;
// }

// struct ntb_ring *
// rte_local_ring_create_custom(struct rte_rawdev *dev,uint64_t ring_size)
// {
// 	struct ntb_ring *r = malloc(sizeof(*r));
// 	struct ntb_hw *hw = dev->dev_private;
// 	//void *xlat_addr;
// 	//uint64_t xlat_off;
// 	//uint64_t base;
// 	//xlat_off = XEON_IMBAR1XBASE_OFFSET + mw_idx * XEON_BAR_INTERVAL_OFFSET;
// 	//limit_off = XEON_IMBAR1XLMT_OFFSET + mw_idx * XEON_BAR_INTERVAL_OFFSET;
// 	//xlat_addr = hw->hw_addr + xlat_off;
// 	//printf("addr = %p,off = %ld\n",xlat_addr,xlat_off);
// 	//limit_addr = hw->hw_addr + limit_off;
// 	//r->start_addr = xlat_addr;
// 	r->ring =((uint8_t*)hw->mz[0]->addr)+16;
// 	r->start_addr = r->ring;
// 	r->end_addr = r->start_addr +ring_size;
// 	r->size = ring_size;
// 	return r;
// }

struct ntb_ring *
rte_ring_create_custom(uint8_t *ptr, uint64_t ring_size)
{
	struct ntb_ring *r = malloc(sizeof(*r));
	//r->ring = intel_ntb_get_peer_mw_addr(*dev, mw_idx);
	r->cur_serial = 0;
	r->start_addr = ptr;
	r->end_addr = r->start_addr + ring_size;
	r->capacity = ring_size / MAX_NTB_MSS_LEN;
	printf("ring_capacity == %ld\n", r->capacity);
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
ntb_custom_start(uint16_t dev_id)
{
	struct rte_rawdev *dev;
	printf("Start dev_id=%d\n", dev_id);

	dev = &rte_rawdevs[dev_id];

	struct ntb_custom_link *ntb_link = malloc(sizeof(*ntb_link));
	ntb_link->dev = dev;
	ntb_link->hw = dev->dev_private;
	ntb_link->sublink = malloc(sizeof(struct ntb_custom_sublink) * PRIORITY);
	int diag;

	if (dev->started != 0)
	{
		printf("Device with dev_id=%d already started",
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
	printf("Start link successed!!\n");
	return ntb_link;
}