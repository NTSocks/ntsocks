/*
 * <p>Title: main.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 14, 2019
 * @version 1.0
 */

#include <getopt.h>
#include <string.h>
#include "ntb_monitor.h"
#include "nt_log.h"
#include "config.h"
#include "nts_shm.h"
#include "ntm_msg.h"

// DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

//#define CONFIG_FILE "/etc/ntm.cfg"
#define CONFIG_FILE "./ntm.cfg"

static void usage(const char *argv0){
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, " %s            start a server and wait for connection\n", argv0);
	fprintf(stdout, "  %s <host>     connect to server at <host>\n", argv0);
	fprintf(stdout, "\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "  -h, --help        	  	print the help information\n");
	fprintf(stdout, "  -s, --server 			listen ip addr\n");
	fprintf(stdout, "  -p, --port 			 listen on/connect to port\n");
	fprintf(stdout, "  -c, --conf			 monitor config file path\n");
}


void test_nts_shm() {
	nts_msg msg;

	char *nts_shm_name = "nts-test";

	nts_shmring_handle_t handle;
	handle = nts_shmring_init(nts_shm_name, strlen(nts_shm_name));
	int i;
	bool ret;
	for ( i = 0; i < 10; i++)
	{
		ret = nts_shmring_pop(handle, &msg);
		while (!ret)
		{
			// sched_yield();
			ret = nts_shmring_pop(handle, &msg);
		}
		printf("msg_id=%d, msg_type=%d, sockid=%d, retval=%d\n", 
					msg.msg_id, msg.msg_type, msg.sockid, msg.retval);
		
	}

	nts_shmring_free(handle, 1);

}


int main(int argc, char **argv) {
	// print_monitor();
	char *conf_file;
	
	while(1){
		int c;
		static struct option long_options[] = {
			{.name = "help", .has_arg = 0, .val = 'h'},
			{.name = "server", .has_arg = 1, .val = 's'},
			{.name = "port", .has_arg = 1, .val = 'p'},
			{.name = "conf", .has_arg = 1, .val = 'c'},
			{.name = NULL,	.has_arg = 0, .val='\0'}
		};
		c = getopt_long(argc, argv, "h:s:p:c:", long_options, NULL);
		if(c == -1)
			break;
		
		switch (c)
		{
		case 'h':
			usage(argv[0]);
			return 1;
		case 's':
			NTM_CONFIG.listen_ip = strdup(optarg);
			break;
		case 'p':
			NTM_CONFIG.listen_port = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			conf_file = strdup(optarg);
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	// DEBUG("before load conf");
	// print_conf();

	// load_conf(CONFIG_FILE);
	// DEBUG("after load conf");
	// print_conf();

	// test_nts_shm();

	ntm_init(CONFIG_FILE);

	getchar();

	ntm_destroy();

	return 0;
}
