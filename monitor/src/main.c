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

#define CONFIG_FILE "ntm.cfg"

int main(int argc, char **argv) {
	printf("Test llllll\n");
	// print_monitor();
	ntm_init(CONFIG_FILE);

	getchar();

	ntm_destroy();

	return 0;
}
