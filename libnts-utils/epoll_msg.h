#ifndef __EPOLL_MSG__H__
#define __EPOLL_MSG__H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define EPOLL_SHM_NAME_LEN 64

/* define the shm address of ntp epoll send/recv queue */
#define NTP_EP_RECV_QUEUE "ntp_epoll_recv_queue"
#define NTP_EP_SEND_QUEUE "ntp_epoll_send_queue"

    typedef uint64_t epoll_msg_id;

    /*----------------------------------------------------------------------------*/
    /**
     * the message definition for epoll shm ringbuffer:
     * sender: libnts, ntm, ntp
     * receiver: libnts, ntm, ntp
     */
    typedef enum
    {
        EPOLL_MSG_CREATE = 1 << 0,
        EPOLL_MSG_CTL = 1 << 1,
        EPOLL_MSG_CLOSE = 1 << 2,
        EPOLL_MSG_ERR = 1 << 3
    } epoll_msg_type;

    typedef enum
    {
        EPOLL_SUCCESS = 1 << 0,
        EPOLL_FAILED = 1 << 1,
    } epoll_status;

    typedef struct _epoll_msg
    {
        epoll_msg_id id;
        epoll_msg_type msg_type;
        epoll_status status;
        int sockid;
        int sock_type;
        int retval;   // if 0, pass; else -1, failed
        int nt_errno; // conresponding to error type
        int shm_addrlen;
        int epid;          // for epoll socket id, NTM_MSG_EPOLL_CTL
        int epoll_op;      // NTS_EPOLL_CTL_ADD, NTS_EPOLL_CTL_MOD, NTS_EPOLL_CTL_DEL
        int io_queue_size; // for epoll ready I/O queue size
        uint64_t ep_data;  // NTM_MSG_EPOLL_CTL
        uint16_t src_port;
        uint16_t dst_port;
        uint32_t events; // NTM_MSG_EPOLL_CTL

        char shm_name[EPOLL_SHM_NAME_LEN];
    } epoll_msg;

#define EPOLL_MSG_SIZE sizeof(struct _epoll_msg)
#define DEFAULT_MAX_NUM_MSG 16

#ifdef __cplusplus
};
#endif
#endif //!__EPOLL_MSG__H__