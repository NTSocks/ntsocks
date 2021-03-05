/*
 * <p>Title: ntm_msg.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef NTM_MSG_H_
#define NTM_MSG_H_

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
    uint64_t src_ip;
    uint64_t dst_ip;
    uint64_t src_port;
    uint64_t dst_port;
    
    uint8_t msg_type;
    int8_t partition_id;   // the corresponding partition id for specified connection
    uint16_t msg_len;
    char msg[16];
} __attribute__((packed)) ntp_ntm_msg;

void ntp_ntm_msgcopy(
        ntp_ntm_msg *src_msg, ntp_ntm_msg *target_msg);

typedef struct {
    uint64_t src_ip;
    uint64_t dst_ip;
    uint64_t src_port;
    uint64_t dst_port;
    
    uint8_t msg_type;       //msg_type==1:create conn
    int8_t partition_id;   // the corresponding partition id for specified connection
    uint16_t msg_len;       //It can be any value, it doesn't matter
    char msg[16];           //It can be any value, it doesn't matter
}__attribute__((packed)) ntm_ntp_msg;

void ntm_ntp_msgcopy(
        ntm_ntp_msg *src_msg, ntm_ntp_msg *target_msg);

#ifdef __cplusplus
};
#endif

#endif /* NTM_MSG_H_ */
