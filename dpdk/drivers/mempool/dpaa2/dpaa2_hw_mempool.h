/* SPDX-License-Identifier: BSD-3-Clause
 *
 *   Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *   Copyright 2016 NXP
 *
 */

#ifndef _DPAA2_HW_DPBP_H_
#define _DPAA2_HW_DPBP_H_

#define DPAA2_MAX_BUF_POOLS	8

#define DPAA2_INVALID_MBUF_SEQN	0

struct buf_pool_cfg {
	void *addr;
	/**< The address from where DPAA2 will carve out the buffers */
	rte_iova_t phys_addr;
	/**< Physical address of the memory provided in addr */
	uint32_t num;
	/**< Number of buffers */
	uint32_t size;
	/**< Size including headroom for each buffer */
	uint16_t align;
	/**< Buffer alignment (in bytes) */
	uint16_t bpid;
	/**< Autogenerated buffer pool ID for internal use */
};

struct buf_pool {
	uint32_t size; /**< Size of the Pool */
	uint32_t num_bufs; /**< Number of buffers in Pool */
	uint16_t bpid; /**< Pool ID, from pool configuration */
	uint8_t *h_bpool_mem; /**< Internal context data */
	struct dpaa2_dpbp_dev *dpbp_node; /**< Hardware context */
};

/*!
 * Buffer pool list configuration structure. User need to give DPAA2 the
 * valid number of 'num_buf_pools'.
 */
struct dpaa2_bp_list_cfg {
	struct buf_pool_cfg buf_pool; /* Configuration of each buffer pool*/
};

struct dpaa2_bp_list {
	struct dpaa2_bp_list *next;
	struct rte_mempool *mp; /**< DPDK RTE EAL pool reference */
	int32_t dpaa2_ops_index; /**< Index into DPDK Mempool ops table */
	struct buf_pool buf_pool;
};

struct dpaa2_bp_info {
	uint32_t meta_data_size;
	uint32_t bpid;
	struct dpaa2_bp_list *bp_list;
};

#define mempool_to_bpinfo(mp) ((struct dpaa2_bp_info *)(mp)->pool_data)
#define mempool_to_bpid(mp) ((mempool_to_bpinfo(mp))->bpid)

extern struct dpaa2_bp_info *rte_dpaa2_bpid_info;

int rte_dpaa2_mbuf_alloc_bulk(struct rte_mempool *pool,
		       void **obj_table, unsigned int count);

#endif /* _DPAA2_HW_DPBP_H_ */
