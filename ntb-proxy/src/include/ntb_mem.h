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

#define PRIORITY 8
#define RING_SIZE 0x800000

#define NTB_DATA_MSS_TL 128
#define NTB_HEADER_LEN 6
#define UPDATE_CUM_LINE 500
#define REV_MSS_COUNTER 1000
#define MEM_NODE_HEADER_LEB 6

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
struct ntb_ctrl_link
{
    //remote cum ptr,write to local
	uint64_t *local_cum_ptr;
	//local cum ptr,wirte to remote
	uint64_t *remote_cum_ptr;
	struct ntb_ring *local_ring;
	struct ntb_ring *remote_ring;
	// char *local_ctrl_mss;
	// char *remote_ctrl_mss;
	// uint8_t *local_send_flag;
	// uint8_t *remote_rev_flag;
};

typedef struct rte_ring* ntp_send_ring;

typedef struct ntp_ring_list_node
{
    ntp_send_ring ring;
    ntp_ring_list_node* next_node;
}ntp_ring_list_node;

struct ntb_sublink
{
	//remote cum ptr,write to local
	uint64_t *local_cum_ptr;
	//local cum ptr,wirte to remote
	uint64_t *remote_cum_ptr;
	struct ntb_ring *local_ring;
	struct ntb_ring *remote_ring;
};

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
	uint8_t mss_type;
	uint8_t mss_len;
};

//one message length is 128B
struct ntb_data_message
{
	struct ntb_message_header header;
	char mss[NTB_DATA_MSS_TL-NTB_HEADER_LEN];
};


//one message length is 128B
struct ntb_ctrl_message
{
	struct ntb_message_header header;
	char mss[NTB_DATA_MSS_TL-NTB_HEADER_LEN];
};

int ntb_app_mempool_get(struct rte_mempool *mp, void **obj_p, struct ntb_uuid *app_uuid);

int ntb_set_mw_trans(struct rte_rawdev *dev, const char *mw_name, int mw_idx, uint64_t mw_size);

// int
// ntb_set_mw_trans_custom(struct rte_rawdev *dev,struct rte_memzone *mz,int mw_idx);

int ntb_mss_add_header(struct ntb_custom_message *mss, uint16_t process_id, int payload_len, bool eon);

int ntb_mss_enqueue(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

int ntb_mss_dequeue(struct ntb_custom_sublink *sublink, struct ntb_buff *rev_buff);

// int ntb_send(struct ntb_custom_sublink *sublink, uint16_t process_id);

int ntb_send(struct ntb_custom_sublink *sublink, void *mp_node);

int ntb_receive(struct ntb_custom_sublink *sublink, struct rte_mempool *recevie_message_pool);

struct ntb_ring *
rte_ring_create_custom(uint8_t *ptr, uint64_t ring_size);

int ntb_creat_all_sublink(struct ntb_custom_link *ntb_link, uint8_t *local_ptr, uint8_t *remote_ptr);

struct ntb_custom_link *
ntb_start(uint16_t dev_id);


#endif /* NTB_PROXY_H_ */
