/*
 * <p>Title: ntp-config.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7
 * @date Nov 19, 2019 
 * @version 1.0
 */

#ifndef NTP_CONFIG_H_
#define NTP_CONFIG_H_

#include <stdint.h>

struct ntp_config {
    char *filename;
    struct {
        uint16_t bar_size;
        uint8_t server_number;
        uint8_t sublink_number;
        uint16_t send_ring_size;
        uint16_t recv_ring_size;
        uint16_t idle_sleep;
        uint8_t send_thread_number;
        uint8_t recv_thread_number;
    } nt_proxy;
};

struct ntp_config * get_ntp_config();

#endif /* NTP_CONFIG_H_ */
