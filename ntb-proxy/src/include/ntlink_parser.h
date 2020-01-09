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

char *create_conn_name(uint16_t src_port, uint16_t dst_port);

int index_ctrl_handler(struct ntb_link *ntlink, struct ntb_ctrl_msg *msg);

int detect_pkg_handler(struct ntb_link *ntlink, struct ntb_data_msg *msg);

// struct ntp_rs_ring ntp_rsring_lookup(uint16_t src_port,uint16_t dst_port);

// struct ntp_rs_ring ntp_shmring_lookup();

#endif /* _NTLINK_PARSER_H_ */