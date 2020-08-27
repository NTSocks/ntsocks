/*
 * <p>Title: ntm_msg.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef NTP_NTM_MSG_H_
#define NTP_NTM_MSG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// define for ntp_ntm_shmring, ntm_ntp_shmring
#define NTP_MAX_BUFS 1024 // define NTS_MAX_BUFS as [(a power of 2) -1] (65535 in our case)
#define NTP_BUF_SIZE 256


typedef enum ntp_shm_stat
{
	SHM_UNREADY = 0,
	SHM_READY,
	SHM_CLOSE,
	SHM_UNLINK
} ntp_shm_stat;


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
    uint64_t src_ip;    // listen_ip
    uint64_t dst_ip;    // client_ip
    uint64_t src_port;
    uint64_t dst_port;
    
    uint8_t msg_type;
    int8_t partition_id;   // the corresponding partition id for specified connection
    uint16_t msg_len;
    char msg[16];
}__attribute__((packed)) ntp_ntm_msg;

void ntp_ntm_msgcopy(ntp_ntm_msg *src_msg, ntp_ntm_msg *target_msg);

typedef struct {
    uint64_t src_ip;
    uint64_t dst_ip;
    uint64_t src_port;
    uint64_t dst_port;
    
    uint8_t msg_type;
    int8_t partition_id;   // the corresponding partition id for specified connection
    uint16_t msg_len;
    char msg[16];
}__attribute__((packed)) ntm_ntp_msg;

void ntm_ntp_msgcopy(ntm_ntp_msg *src_msg, ntm_ntp_msg *target_msg);

#ifdef __cplusplus
};
#endif

#endif /* NTP_NTM_MSG_H_ */
