/*
 * <p>Title: ntb_mem.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang, Jing7
 * @date Nov 12, 2019 
 * @version 1.0
 */

#ifndef NTB_MEM_H_
#define NTB_MEM_H_

// #define PRIORITY 8
#define RING_SIZE 0x800000
#define CTRL_RING_SIZE 0x40000

#define NTB_DATA_MSG_TL 128
#define NTB_CTRL_MSG_TL 16
#define NTB_HEADER_LEN 6
#define MEM_NODE_HEADER_LEN 6

struct ntb_ring
{
	uint64_t cur_serial;
	uint8_t *start_addr;
	uint8_t *end_addr;
	uint64_t capacity;
};

struct ntb_ctrl_link
{
	//remote cum ptr,write to local
	uint64_t *local_cum_ptr;
	//local cum ptr,wirte to remote
	uint64_t *remote_cum_ptr;
	struct ntb_ring *local_ring;
	struct ntb_ring *remote_ring;
};

struct ntb_sublink
{
	//remote cum ptr,write to local
	uint64_t *local_cum_ptr;
	//local cum ptr,wirte to remote
	uint64_t *remote_cum_ptr;
	struct ntb_ring *local_ring;
	struct ntb_ring *remote_ring;
};


typedef struct rte_ring* ntp_rs_ring;

typedef struct ntp_ring_list_node
{
    ntp_rs_ring ring;
    ntp_ring_list_node* next_node;
}ntp_ring_list_node;

struct ntb_link
{
	struct rte_rawdev *dev;
	struct ntb_hw *hw;

	ntp_ring_list_node *send_ring_head;

	struct ntb_ctrl_link *ctrllink;
	struct ntb_sublink *sublink;
};

//header length is 6B
struct ntb_message_header
{
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t msg_len;
};

//one message length is 128B
struct ntb_data_msg
{
	struct ntb_message_header header;
	char msg[NTB_DATA_MSG_TL - NTB_HEADER_LEN];
};

//one message length is 16B
struct ntb_ctrl_message
{
	struct ntb_message_header header;
	char msg[NTB_CTRL_MSG_TL - NTB_HEADER_LEN];
};

int ntb_app_mempool_get(struct rte_mempool *mp, void **obj_p, struct ntb_uuid *app_uuid);

int ntb_send_data(struct ntb_sublink *sublink, void *mp_node, uint16_t src_port, uint16_t dst_port);

int ntb_receive(struct ntb_sublink *sublink, struct rte_mempool *recevie_message_pool);

struct ntb_link *ntb_start(uint16_t dev_id);

#endif /* NTB_PROXY_H_ */
