/*
 * <p>Title: config.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 16, 2019 
 * @version 1.0
 */

#include "ntb_monitor.h"
#include "config.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);


struct ntm_config NTM_CONFIG = {
		/* set default configuration */
		.remote_ntm_tcp_timewait = 0,
		.remote_ntm_tcp_timeout = 1000
};

ntm_manager_t ntm_mgr = NULL;


int load_conf(const char *fname) {
	DEBUG("load ntb monitor configuration from conf file (%s)", fname);

	return 0;
}


void print_conf() {

}


