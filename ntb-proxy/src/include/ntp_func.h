/*
 * <p>Title: ntp_func.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7
 * @date Nov 12, 2019 
 * @version 1.0
 */

#ifndef NTP_FUNC_H_
#define NTP_FUNC_H_

#include "ntb_mw.h"
#include "ntp_nts_msg.h"
#include "ntp_nts_shm.h"
#include "ntm_msg.h"
#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"

#define CREATE_CONN_ACK 1

#define NTP_DATA 1
#define NTP_FIN 2

#define DETECT_INTERVAL 4   //Unit : US

#define RTE_GET_TIMER_HZ 25000000000

int ntp_create_conn_handler(struct ntb_link *ntb_link, ntm_ntp_msg *msg);

int ntp_send_buff_data(struct ntb_data_link *data_link, ntp_shm_context_t ring, ntb_conn *conn);

int ntp_receive_data_to_buff(struct ntb_data_link *data_link, struct ntb_link *ntb_link);

int ntp_ctrl_msg_receive(struct ntb_link *ntb_link);

#endif /* NTP_FUNC_H_ */