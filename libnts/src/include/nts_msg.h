/*
 * <p>Title: nts_msg.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTS_MSG_H_
#define NTS_MSG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t nts_msg_id;
typedef uint64_t ntsock_id;

typedef enum {
    NTS_MSG_INIT = 1 << 0,
    NTS_MSG_NEW_SOCK = 1 << 1,
    NTS_MSG_BIND = 1 << 2,
    NTS_MSG_LISTEN = 1 << 3,
    NTS_MSG_ACCEPT = 1 << 4,
    NTS_MSG_CONNECT = 1 << 5,
    NTS_MSG_ESTABLISH = 1 << 6,
    NTS_MSG_CLOSE = 1 << 7,
    NTS_MSG_DISCONNECT = 1 << 8
} nts_msg_type;


typedef struct {
    nts_msg_id msg_id;
    nts_msg_type msg_type;

    char address[16];
    uint32_t addrlen;
    uint64_t port;

    ntsock_id sockid;
} nts_msg;

void nts_msgcopy(nts_msg *src_msg, nts_msg *target_msg);



#ifdef __cplusplus
};
#endif

#endif /* NTS_MSG_H_ */
