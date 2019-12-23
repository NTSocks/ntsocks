#include "ntp_config.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

struct ntp_config *pconfig;

static int
ini_parse_handler(void* user, const char* section, const char* name,
    const char* value)
{
    struct ntp_config *pconfig = (struct ntp_config*)user;

    printf("[%s]: %s=%s\n", section, name, value);

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("nt-proxy", "bar_size")) {
        pconfig->nt_proxy.bar_size = atoi(value);
    } else if (MATCH("nt-proxy", "server_number")) {
        pconfig->nt_proxy.server_number = atoi(value);
    } else if (MATCH("nt-proxy", "sublink_number")) {
        pconfig->nt_proxy.sublink_number = atoi(value);
    } else if (MATCH("nt-proxy", "send_ring_size")) {
        pconfig->nt_proxy.send_ring_size = strdup(value);
        return parse_lcore_mask(pconfig, pconfig->dpdk.lcore_mask);
    } else if (MATCH("nt-proxy", "recv_ring_size")) {
        pconfig->nt_proxy.recv_ring_size= strdup(value);
    } else if (MATCH("nt-proxy", "port_list")) {
        return parse_port_list(pconfig, value);
    } else if (MATCH("nt-proxy", "idle_sleep")) {
        pconfig->nt_proxy.idle_sleep = atoi(value);
    } else if (MATCH("nt-proxy", "send_thread_number")) {
        pconfig->nt_proxy.send_thread_number = atoi(value);
    } else if (MATCH("nt-proxy", "recv_thread_number")) {
        pconfig->nt_proxy.recv_thread_number = atoi(value);
    }
    return 1;
}


struct ntp_config *
get_ntp_config()
{
    if (pconfig != NULL){
        return pconfig;
    }else
    {
        ERR("ntm_shm_nts_close pass \n")
    }
    return 0;
}