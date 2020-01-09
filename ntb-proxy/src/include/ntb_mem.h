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

#include "nts_shm.h"
#include "hash_map.h"

#define DETECT_PKG 0
#define SINGLE_PKG 1
#define MULTI_PKG 2
#define ENF_MULTI 3
#define FIN_PKG 4

// #define PRIORITY 8
#define RING_SIZE 0x800000
#define CTRL_RING_SIZE 0x40000

#define NTB_DATA_MSG_TL 128
#define NTB_CTRL_MSG_TL 16
#define NTB_HEADER_LEN 6
#define MEM_NODE_HEADER_LEN 6

#define READY_CONN 1
#define CLOSE_CONN 2

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

// typedef ntp_shm_context_t *ntp_rs_ring;

typedef struct ntp_ring_list_node
{
	ntb_conn *conn;
	ntp_ring_list_node *next_node;
} ntp_ring_list_node;

typedef struct ntb_conn_context
{
	uint8_t state;
	char name[14];
	ntp_shm_context_t send_ring;
	ntp_shm_context_t recv_ring;
} ntb_conn;
typedef struct ntb_conn_context
{
	uint8_t state;
	char name[14];
	ntp_shm_context_t send_ring;
	ntp_shm_context_t recv_ring;
} ntb_conn;

struct ntb_link
{
	struct rte_rawdev *dev;
	struct ntb_hw *hw;
	HashMap map;
	nts_shm_context_t ntm_ntp;
	nts_shm_context_t ntp_ntm;
	ntp_ring_list_node *ring_head;
	ntp_ring_list_node *ring_tail;

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
struct ntb_ctrl_msg
{
	struct ntb_message_header header;
	char msg[NTB_CTRL_MSG_TL - NTB_HEADER_LEN];
};

int add_conn_to_ntlink(struct ntb_link *link, ntp_shm_context_t ring);

int ntb_send_data(struct ntb_sublink *sublink, void *mp_node, uint16_t src_port, uint16_t dst_port);

int ntb_receive(struct ntb_sublink *sublink, struct ntb_link *ntlink);

struct ntb_link *ntb_start(uint16_t dev_id);

#endif /* NTB_PROXY_H_ */
