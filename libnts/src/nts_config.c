/*
 * <p>Title: nts_config.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */


#include "nts.h"
#include "nts_config.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);


struct nts_config NTS_CONFIG = {
		/* set default configuration */
		.tcp_timewait = 0,
		.tcp_timeout = 1000
};

nts_context_t nts_ctx = NULL;


int load_conf(const char *fname) {
	DEBUG("load libnts configuration from conf file (%s)", fname);

	return 0;
}


void print_conf() {

}
