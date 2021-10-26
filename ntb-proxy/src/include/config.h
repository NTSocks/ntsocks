#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define NTM_LISTEN_IP "0.0.0.0"
#define NTM_LISTEN_PORT 9090

#define CONFIG_FILE "/etc/ntp.cfg"

#define MAX_BUF_LEN 1024
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 256
#define DEFAULT_NTPACKET_SIZE 7
#define DEFAULT_CTRL_PACKET_SIZE 16
#define DEFAULT_DATA_RING_SIZE 0x4000000 // 8MB --> 64MB
#define DEFAULT_CTRL_RING_SIZE 0x40000	 // 256KB
#define MAX_NUM_PARTITION 8

#define DEFAULT_MTU_SIZE 1024
#define DEFAULT_BULK_SIZE 16
#define DEFAULT_NUM_PARTITION 1
#define DEFAULT_CFG_PATH "/etc/ntp.cfg"

    struct ntp_config
    {
        int sublink_data_ring_size;
        int sublink_ctrl_ring_size;
        int nts_buff_size;
        int bulk_size;

        uint16_t num_partition;
        uint16_t ntb_packetbits_size;  // if value is 7, it means
                                       //  the packet size of NTB data ringbuffer is 1 << 7 (128)
        uint16_t ctrl_packet_size;     // default 16B
        uint64_t data_ringbuffer_size; // default 8MB, expected 128MB
        uint64_t ctrl_ringbuffer_size; // default 256B, expected 1MB

        uint16_t data_packet_size;        // default 1 KB
        uint16_t datapacket_payload_size; // = data_packet_size - DATA_PACKET_HEADER_SIZE
    };

    extern char ntp_cfg_path[256];
    extern struct ntp_config NTP_CONFIG;

    /* load configuration from specified configuration file name */
    int load_conf(const char *fname);

    /* print setted configuration */
    void print_conf(void);

    int math_log2(int value);

#ifdef __cplusplus
};
#endif

#endif /* CONFIG_H_ */
