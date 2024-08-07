/*
 * <p>Title: ntp_nts_msg.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef NTP_NTS_MSG_H_
#define NTP_NTS_MSG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/**
 * the message definition for the communication
 * between local and remote ntb-monitor.
 */

#define PAYLOAD_SIZE 1024


/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */


typedef struct {
    uint8_t msg_type;
    uint16_t msg_len;
    char msg[253];
} ntp_msg;

void ntp_msgcopy(ntp_msg *src_msg, ntp_msg *target_msg);


#ifdef __cplusplus
};
#endif

#endif /* NTP_NTS_MSG_H_ */
