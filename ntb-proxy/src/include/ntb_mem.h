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

#include "hash_map.h"
#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"
#include "ntp_nts_shm.h"

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

#define SEND_DETECT 4











//header length is 6B

int parser_conn_name(char *conn_name, uint16_t *src_port, uint16_t *dst_port);

int add_conn_to_ntlink(struct ntb_link *link, ntb_conn *conn);

int ntb_data_send(struct ntb_data_link *data_link, ntp_shm_context_t ring, struct ntb_link *ntlink, uint64_t *counter);

int ntb_data_receive(struct ntb_data_link *data_link, struct ntb_link *ntlink, uint64_t *counter);

#endif /* NTB_MEM_H_ */