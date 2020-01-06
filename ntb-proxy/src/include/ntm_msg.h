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

typedef uint64_t ntm_msg_id;
typedef uint64_t nts_msg_id;
typedef uint64_t ntsock_id;



/*----------------------------------------------------------------------------*/
/**
 * the message definition for the communication
 * between local and remote ntb-monitor.
 */

#define PAYLOAD_SIZE 1024

typedef enum {
    CONNECT_OK = 1,
    TRACK = 10,
    TRACK_CONFIRM = 15,
    STOP = 20,
    STOP_CONFIRM = 25,
    SUCCESS = 30,
    FAILURE = 40,
    QUIT = 50
} ntm_sock_msg_type;

typedef struct {
	ntm_sock_msg_type type;
    char payload[PAYLOAD_SIZE];
} ntm_sock_msg;

typedef struct {
    int human_id;
    int torso_height;
    int arm_length;
    int leg_length;
    int head_diameter;
} human_data;


/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */

typedef enum {
    NTM_MSG_INIT = 1 << 0,
    NTM_MSG_NEW_SOCK = 1 << 1,
    NTM_MSG_BIND = 1 << 2,
    NTM_MSG_LISTEN = 1 << 3,
    NTM_MSG_ACCEPT = 1 << 4,
    NTM_MSG_CONNECT = 1 << 5,
    NTM_MSG_ESTABLISH = 1 << 6,
    NTM_MSG_CLOSE = 1 << 7,
    NTM_MSG_DISCONNECT = 1 << 8
} ntm_msg_type;


typedef struct {
    // ntm_msg_id msg_id;
    ntm_msg_type msg_type;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
} ntm_msg;

void ntm_msgcopy(ntm_msg *src_msg, ntm_msg *target_msg);



/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */
// typedef enum {
//     NTS_MSG_DATA = 1 << 0,
// } nts_msg_type;


typedef struct {
    uint8_t msg_type;
    uint16_t msg_len;
    char msg[253];
} ntp_msg;

void nts_msgcopy(nts_msg *src_msg, nts_msg *target_msg);


#ifdef __cplusplus
};
#endif

#endif /* NTM_MSG_H_ */
