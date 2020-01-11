/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */
#ifndef _NTB_PROTOCOLS_H_
#define _NTB_PROTOCOLS_H_

#include "ntb_custom.h"

#define DATA_TYPE 1
#define OPEN_LINK 10
#define OPEN_LINK_ACK 11
#define OPEN_LINK_FAIL 19
#define FIN_LINK 20
#define FIN_LINK_ACK 21
#define NTB_BUFF_SIZE 0x400000



int ntb_buff_creat(struct ntb_custom_sublink *sublink, int process_id);

int ntb_trans_cum_ptr(struct ntb_custom_sublink *sublink);

int ntb_send_open_link(struct ntb_custom_sublink *sublink, uint16_t process_id);

int ntb_send_fin_link(struct ntb_custom_sublink *sublink, uint16_t process_id);

int ntb_open_link_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

int ntb_open_link_ack_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

int ntb_open_link_fail_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

// int ntb_fin_link_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

int ntb_fin_link_ack_handler(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

int ntb_prot_header_parser(struct ntb_custom_sublink *sublink, struct ntb_custom_message *mss);

#endif /* _NTB_PROTOCOLS_H_ */