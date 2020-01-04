/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */
#ifndef _NTB_CUSTOM_H_
#define _NTB_CUSTOM_H_

#include <rte_ring.h>

#define PRIORITY 8
#define RING_SIZE 0x800000
#define MAX_NTB_MSS_LEN 128
#define NTB_HEADER_LEN 4
#define UPDATE_CUM_LINE 500
#define REV_MSS_COUNTER 1000

struct ntb_buff
{
	//struct rte_ring *ring;
    uint8_t *buff;
    uint64_t buff_size;
    uint64_t data_len;
};

struct ntb_socket
{
    uint8_t occupied;
    struct ntb_buff *rev_buff;
    struct ntb_buff *send_buff;
};

struct ntb_ring
{
	uint64_t cur_serial;
	uint8_t *start_addr;
	uint8_t *end_addr;
	uint64_t capacity;
};

// struct ntb_custom_ops {
// 	int (*mw_set_trans)(struct rte_rawdev *dev, int mw_idx,
// 			    uint64_t addr, uint64_t size);
// 	int (*set_link)(struct rte_rawdev *dev, bool up);
// };
struct ntb_ctrl_mss
{
	char *local_ctrl_mss;
	char *remote_ctrl_mss;
	uint8_t *local_send_flag;
	uint8_t *remote_rev_flag;
};

struct ntb_custom_sublink
{
	struct ntb_socket *process_map;
	//remote cum ptr,write to local
	uint64_t *local_cum_ptr;
	//local cum ptr,wirte to remote
	uint64_t *remote_cum_ptr;
	struct ntb_ctrl_mss *ctrl_mss;
	struct ntb_ring *local_ring;
	struct ntb_ring *remote_ring;
};

struct ntb_custom_link
{
	struct rte_rawdev *dev;
	struct ntb_hw *hw;
	//	const struct ntb_custom_ops *custom_ops;
	//bitmap record the status of process id
	//uint8_t process_map[0x2000];
	struct ntb_custom_sublink *sublink;

	// struct ntb_ring *local_ring;
	// struct ntb_ring *remote_ring;
};

//header length is 4B
struct ntb_message_header
{
	uint16_t process_id;
	uint8_t mss_type;
	uint8_t mss_len;
};

//one message length is 128B
struct ntb_custom_message
{
	struct ntb_message_header header;
	char mss[124];
};

int ntb_set_mw_trans(struct rte_rawdev *dev, const char *mw_name, int mw_idx, uint64_t mw_size);

// int
// ntb_set_mw_trans_custom(struct rte_rawdev *dev,struct rte_memzone *mz,int mw_idx);

int ntb_mss_add_header(struct ntb_custom_message *mss, uint16_t process_id, int payload_len);

int ntb_mss_enqueue(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

int ntb_mss_dequeue(struct ntb_custom_sublink *sublink, struct ntb_buff *rev_buff);

int ntb_send(struct ntb_custom_sublink *sublink, uint16_t process_id);

struct ntb_ring *
rte_ring_create_custom(uint8_t *ptr, uint64_t ring_size);

int ntb_creat_all_sublink(struct ntb_custom_link *ntb_link, uint8_t *local_ptr, uint8_t *remote_ptr);

struct ntb_custom_link *
ntb_custom_start(uint16_t dev_id);

#endif /* _NTB_CUSTOM_H_ */