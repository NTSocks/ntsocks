/*
 * <p>Title: ntp2nts_msg.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef NTP2NTS_MSG_H_
#define NTP2NTS_MSG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/**
 * the message definition for the communication
 * between local and remote ntb-monitor.
 */

#define PAYLOAD_SIZE            1024
#define NTP_PAYLOAD_MAX_SIZE    252


/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */

/* define the message type between ntp and libnts */
#define NTP_NTS_MSG_DATA        1
#define NTP_NTS_MSG_FIN         2       

typedef struct {
    uint16_t msg_type;   // 2 bytes
    uint16_t msg_len;   // 2 bytes
    char msg[NTP_PAYLOAD_MAX_SIZE]; // 252 bytes
} ntp_msg;

void ntp_msgcopy(ntp_msg *src_msg, ntp_msg *target_msg);

#define NTP_MSG_IDX_SIZE sizeof(int)
#define DEFAULT_MAX_NUM_NTP_MSG 1024

#ifdef __cplusplus
};
#endif

#endif /* NTP2NTS_MSG_H_ */