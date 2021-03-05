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
#include <signal.h>

#include "ntb_monitor.h"
#include "config.h"
#include "nts_shm.h"
#include "ntm_msg.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

//#define CONFIG_FILE "/etc/ntm.cfg"
#define CONFIG_FILE "./ntm.cfg"

static void usage(const char *argv0)
{
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, " %s            start a server and wait for connection\n", argv0);
	fprintf(stdout, " %s <host>     connect to server at <host>\n", argv0);
	fprintf(stdout, "\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "  -h, --help        	 print the help information\n");
	fprintf(stdout, "  -s, --server 		 listen ip addr\n");
	fprintf(stdout, "  -p, --port 			 listen on/connect to port\n");
	fprintf(stdout, "  -c, --conf			 monitor config file path\n");
}

void ntm_on_exit(void)
{
	INFO("destroy all ntm resources when on exit.");
	ntm_destroy();
}

void signal_crash_handler(int sig)
{
	exit(-1);
}

void signal_exit_handler(int sig)
{
	exit(0);
}

int main(int argc, char **argv)
{
	atexit(ntm_on_exit);
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);

	signal(SIGBUS, signal_crash_handler);  // 总线错误
	signal(SIGSEGV, signal_crash_handler); // SIGSEGV，非法内存访问
	signal(SIGFPE, signal_crash_handler);  // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等
	signal(SIGABRT, signal_crash_handler); // SIGABRT，由调用abort函数产生，进程非正常退出

	char *conf_file;

	while (1)
	{
		int c;
		static struct option long_options[] = {
			{.name = "help", .has_arg = 0, .val = 'h'},
			{.name = "server", .has_arg = 1, .val = 's'},
			{.name = "port", .has_arg = 1, .val = 'p'},
			{.name = "conf", .has_arg = 1, .val = 'c'},
			{.name = NULL, .has_arg = 0, .val = '\0'}};
		c = getopt_long(argc, argv, "h:s:p:c:", long_options, NULL);
		if (c == -1)
			break;

		switch (c)
		{
		case 'h':
			usage(argv[0]);
			return -1;
		case 's':
			NTM_CONFIG.ipaddr_len = strlen(optarg);
			memcpy(NTM_CONFIG.listen_ip, optarg, NTM_CONFIG.ipaddr_len);
			printf("listen_ip:%s \n", NTM_CONFIG.listen_ip);
			break;
		case 'p':
			NTM_CONFIG.listen_port = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			conf_file = (char *)strdup(optarg);
			printf("config file: %s\n", conf_file);
			break;
		default:
			usage(argv[0]);
			return -1;
		}
	}

	if (!conf_file)
	{
		conf_file = CONFIG_FILE;
	}

	ntm_init(conf_file);

	return 0;
}
