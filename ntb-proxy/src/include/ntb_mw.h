/*
 * <p>Title: ntb_mw.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7
 * @date Nov 12, 2019 
 * @version 1.0
 */

#ifndef NTB_MW_H_
#define NTB_MW_H_

#include "hash_map.h"
#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"
#include "ntp_nts_shm.h"

#define DATA_RING_SIZE 0x800000
#define CTRL_RING_SIZE 0x40000

#define NTB_DATA_MSG_TL 128
#define NTB_CTRL_MSG_TL 16
#define NTB_HEADER_LEN 6
#define DATA_MSG_LEN 122 //data_msg_len = NTB_DATA_MSG_TL - NTB_HEADER_LEN

enum ntb_data_msg_type
{
    DETECT_PKG = 0,
    SINGLE_PKG = 1,
    MULTI_PKG = 2,
    ENF_MULTI = 3,
    FIN_PKG = 4
};

struct ntb_message_header
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t msg_len; // 2 bytes | 1 bit(push )  |  3 bit (msg_type)   |  12 bit (msg len)
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

struct ntb_ring_buffer // ntb buffer
{
    uint64_t cur_index;  // read_index on local ntb buffer ring while write_index on peer (ntb buffer)
    uint8_t *start_addr; // start memory address of ntb buffer
    uint8_t *end_addr;   // end memory pointer address of ntb buffer
    uint64_t capacity;   // total capacity of ntb buffer for send/recv data/control message
};

struct ntb_ctrl_link
{
    //remote cum ptr,write to local
    volatile uint64_t *local_cum_ptr;
    //local cum ptr,wirte to remote
    volatile uint64_t *remote_cum_ptr;
    struct ntb_ring_buffer *local_ring;
    struct ntb_ring_buffer *remote_ring;
};

struct ntb_data_link // data ntb link for send/recv data msg
{
    //remote cum ptr,write to local		// read_index direction: peer ==> local
    volatile uint64_t *local_cum_ptr; //	local ntb node record the read_index of ntb buffer on peer ntb node
    //local cum ptr,wirte to remote
    // read_index direction: local ==> peer
    volatile uint64_t *remote_cum_ptr;   // 	[] remote peer ntb node get/poll the read_index of local ntb node
                                         //	the target location (memory address) which is used
                                         // 	to receive local read_index by peer ntb node
    struct ntb_ring_buffer *local_ring;  //	local ring for read/recv data msg from peer node
    struct ntb_ring_buffer *remote_ring; //	remote ring for write/send data msg to peer node
};

enum ntb_connection_state
{
    READY_CONN = 1,
    ACTIVE_CLOSE = 2,
    PASSIVE_CLOSE = 3
};

typedef struct ntb_connection_context
{
    uint8_t state; //	READY = 1, ACTIVE_CLOSE = 2, PASSIVE = 3

    uint64_t detect_time; // the timestamp of send previous detect_pkg

    uint32_t conn_id;                // the unique name/id for ntb connect, format: [src port << 16 + dst port]
    ntp_shm_context_t nts_send_ring; //	the shm-based send ring buffer for ntb connection
    ntp_shm_context_t nts_recv_ring; //	the shm-based recv ring buffer for ntb connection
} ntb_conn;

typedef struct ntp_send_list_node
{
    ntb_conn *conn;
    struct ntp_send_list_node *next_node;
} ntp_send_list_node;

struct ntp_send_list
{
    ntp_send_list_node *ring_head; // head node of list for ntb connect (struct nt_conn) ==> send buffer/ recv buffer
    ntp_send_list_node *ring_tail; // tail node of list for ntb connection ==> for send/recv buffer
};

struct ntb_link
{
    struct rte_rawdev *dev; // the abstract of ntb raw device
    struct ntb_hw *hw;      // the operation methods about ntb device

    HashMap port2conn;             // hash map for the ntb connection:
                                   // key: [src-port]-[dst-port]
                                   // value: ntb connection struct --> struct ntb_conn
    ntm_ntp_shm_context_t ntm_ntp; // ntm ==> ntp shm ring buffer context
    ntp_ntm_shm_context_t ntp_ntm; // ntp ==> ntm shm ring buffer context

    struct ntp_send_list send_list;

    struct ntb_ctrl_link *ctrl_link; // the send/recv buffer for the control message between local and peer ntb nodes
    struct ntb_data_link *data_link; // the send/recv buffer for the data message between local peer ntb nodes
};

int ntb_data_msg_add_header(struct ntb_data_msg *msg, uint16_t src_port, uint16_t dst_port, int payload_len, int msg_type);

int parser_data_len_get_type(struct ntb_data_link *data_link, uint16_t msg_len);

int parser_ctrl_msg_header(struct ntb_link *ntb_link, uint16_t msg_len);

int ntb_ctrl_msg_enqueue(struct ntb_link *ntlink, struct ntb_ctrl_msg *msg);

int ntb_pure_data_msg_enqueue(struct ntb_data_link *data_link, uint8_t *msg, int data_len);

int ntb_data_msg_enqueue(struct ntb_data_link *data_link, struct ntb_data_msg *msg);

//start the ntb device,and return a ntb_link
struct ntb_link *ntb_start(uint16_t dev_id);

#endif /* NTB_MW_H_ */