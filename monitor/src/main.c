/*
 * <p>Title: main.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 14, 2019 
 * @version 1.0
 */

#include "ntb_monitor.h"
#include "nt_log.h"
#include "config.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define CONFIG_FILE "/etc/ntm.cfg"


int main(int argc, char **argv) {
	// print_monitor();
	DEBUG("before load conf");
	print_conf();

	load_conf(CONFIG_FILE);
	DEBUG("after load conf");
	print_conf();

	ntm_init(CONFIG_FILE);

	getchar();

	ntm_destroy();

	return 0;
}
