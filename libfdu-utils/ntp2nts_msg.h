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
extern "C"
{
#endif

/*----------------------------------------------------------------------------*/
/**
 * the message definition for the communication
 * between local and remote ntb-monitor.
 */

// #define NTP_PAYLOAD_MAX_SIZE    252

/*----------------------------------------------------------------------------*/
/**
 * the message definition for ntm-ringbuffer:
 * sender: nts app
 * receiver: ntb-monitor
 */

/* define the message type between ntp and libnts */
#define NTP_NTS_MSG_DATA 1
#define NTP_NTS_MSG_FIN 2

    struct ntpacket_header
    {
        uint16_t src_port;
        uint16_t dst_port;
        uint16_t msg_len;  // 2 bytes | 1 bit(push)  |  3 bit (msg_type)   |  12 bit (msg len)
        uint16_t msg_type; // only used for shm-based communication between nts and ntp: NTP_NTS_MSG_DATA/NTP_NTS_MSG_FIN
    } __attribute__((packed));
    typedef struct ntpacket_header *ntpacket_header_t;

#define SRCPORT_SIZE sizeof(uint16_t)
#define DSTPORT_SIZE sizeof(uint16_t)
#define MSGLEN_SIZE sizeof(uint16_t)
#define EXTEND_SIZE sizeof(uint16_t)

#define SRCPORT_OFFSET 0
#define DSTPORT_OFFSET sizeof(uint16_t)
#define MSGLEN_OFFSET DSTPORT_OFFSET + sizeof(uint16_t)
#define EXTEND_OFFSET MSGLEN_OFFSET + sizeof(uint16_t)

#define NTPACKET_HEADER_LEN sizeof(struct ntpacket_header)

    typedef struct ntpacket
    {
        // ntpacket_header_t header;
        // char *payload;
        struct ntpacket_header header;
        char payload[0];
    } ntp_msg, ntpacket;
    typedef struct ntpacket *ntpacket_t, *ntp_msg_t;

    /**
     *	used by ntp to parse ntpacket from ntb buffer (element of ntb data ringbuffer)
    *		return 0 if success; return -1 if failed
    */
    inline int parse_ntpacket(void *packet_ptr, ntp_msg **buf)
    {
        if (!packet_ptr || !buf)
        {
            return -1;
        }

        *buf = (ntp_msg *)packet_ptr;

        return 0;
    }

#define NTP_MSG_IDX_SIZE sizeof(int)
#define DEFAULT_MAX_NUM_NTP_MSG 2048

#ifdef __cplusplus
};
#endif

#endif /* NTP2NTS_MSG_H_ */
