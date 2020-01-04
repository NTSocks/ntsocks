/*
 * <p>Title: ntlink_parser.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7 
 * @date Nov 19, 2019 
 * @version 1.0
 */

#ifndef _NTLINK_PARSER_H_
#define _NTLINK_PARSER_H_

#include "ntb_mem.h"

#define CONN_REQ = 10,
#define CONN_REQ_ACK = 11,
#define CONN_FAIL = 19,
#define CONN_CLOSE = 20,
#define CONN_CLOSE_ACK = 21

typedef enum ntlink_header_type
{
	CONN_REQ = 10,
	CONN_REQ_ACK = 11,
	CONN_FAIL = 19,
	CONN_CLOSE = 20,
	CONN_CLOSE_ACK = 21
} ntlink_header_type;

struct ntb_ctrl_header
{
	uint16_t src_port;
	uint16_t dst_port;
	uint8_t mss_type;
	uint8_t mss_len;
};

struct ntp_ntm_mss
{
	struct ntb_ctrl_header header;
	char mss[NTB_CTRL_MSS_TL - NTB_HEADER_LEN];
};

struct ntb_ctrl_mss
{
	struct ntb_ctrl_header header;
	char mss[NTB_CTRL_MSS_TL - NTB_HEADER_LEN];
};

struct ntp_rs_ring ntp_rsring_lookup(uint16_t src_port,uint16_t dst_port);

struct ntp_rs_ring ntp_shmring_lookup();

#endif /* _NTLINK_PARSER_H_ */