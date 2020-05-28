/*
 * <p>Title: config.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NTM_LISTEN_IP "127.0.0.1"
#define NTM_LISTEN_PORT 9090

#define CONFIG_FILE "/etc/ntp.cfg"

struct ntp_config
{
    int sublink_number;
    int sublink_data_ring_size;
    int sublink_ctrl_ring_size;
    int nts_buff_size;
    int branch_trans_number;
};

extern struct ntp_config NTP_CONFIG;

/* load configuration from specified configuration file name */
int load_conf(const char *fname);

/* print setted configuration */
void print_conf(void);

#ifdef __cplusplus
};
#endif

#endif /* CONFIG_H_ */
