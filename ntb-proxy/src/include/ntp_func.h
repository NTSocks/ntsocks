#ifndef NTP_FUNC_H_
#define NTP_FUNC_H_

#include "ntb_mw.h"
#include "ntp2nts_msg.h"
#include "ntp2nts_shm.h"
#include "ntm_msg.h"
#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"
#include "utils.h"

#define CREATE_CONN_ACK 1

#define NTP_DATA 1
#define NTP_FIN 2

#define DETECT_INTERVAL 4 // Unit : us
#define DETECT_INTERVAL_TIME (DETECT_INTERVAL * 25000)

#define SHM_NAME_LEN 50

#define RTE_GET_TIMER_HZ 25000000000

int ntp_create_conn_handler(
    struct ntb_link_custom *ntb_link, ntm_ntp_msg *msg);

int ntp_send_buff_data(struct ntb_data_link *data_link,
                       ntb_partition_t partition,
                       ntp_shm_context_t ring, ntb_conn *conn);

int ntp_recv_data_to_buf(struct ntb_data_link *data_link,
                            struct ntb_link_custom *ntb_link,
                            ntb_partition_t partition, 
                            struct ntp_lcore_conf * conf);

int ntp_ctrl_msg_receive(struct ntb_link_custom *ntb_link);

/**
 * For epoll msg handling from ntm  
 * 
 */
int ntp_handle_epoll_msg(
    struct ntb_link_custom *ntb_link, epoll_msg *req_msg);

uint16_t ntp_allocate_partition(struct ntb_link_custom *ntb_link);

#endif /* NTP_FUNC_H_ */