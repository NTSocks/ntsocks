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

#define CREATE_CONN_ACK 1

char *create_conn_name(uint16_t src_port, uint16_t dst_port);


int detect_pkg_handler(struct ntb_link *ntlink, struct ntb_data_msg *msg);

int ctrl_msg_receive(struct ntb_link *ntlink);

int ntp_create_conn_handler(struct ntb_link *ntlink, ntm_ntp_msg *msg);

#endif /* _NTLINK_PARSER_H_ */