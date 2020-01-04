/*
 * <p>Title: config.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Wenxiong Zou
 * @date Dec 16, 2019 
 * @version 1.0
 */

#include "ntb-proxy.h"
#include "config.h"
#include "nt_log.h"
#include <unistd.h>
#include <string.h>

#define MAX_BUF_LEN 1024
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 256
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);


struct ntp_config NTP_CONFIG = {
		/* set default configuration */
		.remote_ntp_tcp_timewait = 0,
		.remote_ntp_tcp_timeout = 1000
};

static int trim(char s[])
{
	int n;
	for (n = strlen(s) - 1; n >= 0; n--) {
		if (s[n] != ' ' && s[n] != '\t' && s[n] != '\n')
			break;
		s[n + 1] = '\0';
	}
	return n;
}

int load_conf(const char *fname)
{
	DEBUG("load ntb proxy configuration from conf file (%s)", fname);
	FILE *file = fopen(fname, "r");
	if (file == NULL) {
		printf("[Error]open %s failed.\n", fname);
		return -1;
	}


	char buf[MAX_BUF_LEN];
	int text_comment = 0;
	while (fgets(buf, MAX_BUF_LEN, file) != NULL) {
		trim(buf);
		// to skip text comment with flags /* ... */
		if (buf[0] != '#' && (buf[0] != '/' || buf[1] != '/')) {
			if (strstr(buf, "/*") != NULL) {
				text_comment = 1;
				continue;
			} else if (strstr(buf, "*/") != NULL) {
				text_comment = 0;
				continue;
			}
		}
		if (text_comment == 1) {
			continue;
		}


		int buf_len = strlen(buf);
		// ignore and skip the line with first chracter '#', '=' or '/'
		if (buf_len <= 1 || buf[0] == '#' || buf[0] == '=' || buf[0] == '/') {
			continue;
		}
		buf[buf_len - 1] = '\0';


		char _paramk[MAX_KEY_LEN] = {0}, _paramv[MAX_VAL_LEN] = {0};
		int _kv = 0, _klen = 0, _vlen = 0;
		int i = 0;
		for (i = 0; i < buf_len; ++i) {
			if (buf[i] == ' ')
				continue;
			// scan param key name
			if (_kv == 0 && buf[i] != '=') {
				if (_klen >= MAX_KEY_LEN)
					break;
				_paramk[_klen++] = buf[i];
				continue;
			} else if (buf[i] == '=') {
				_kv = 1;
				continue;
			}
			// scan param key value
			if (_vlen >= MAX_VAL_LEN || buf[i] == '#')
				break;
			_paramv[_vlen++] = buf[i];
		}
		if (strcmp(_paramk, "") == 0 || strcmp(_paramv, "") == 0)
			continue;
		// DEBUG("ntb proxy configuration %s=%s", _paramk, _paramv);
		if (strcmp(_paramk, "remote_ntp_tcp_timeout") == 0){
			NTP_CONFIG.remote_ntp_tcp_timeout = atoi(_paramv);
		}else if(strcmp(_paramk, "remote_ntp_tcp_timewait") == 0){
			NTP_CONFIG.remote_ntp_tcp_timewait = atoi(_paramv);
		}else {
			return 1;
		}
	}
	return 0;
}

void print_conf() {
	printf("ntp configuration: remote_ntp_tcp_timewait=%d, remote_ntp_tcp_timeout=%d\n",
		NTP_CONFIG.remote_ntp_tcp_timewait, NTP_CONFIG.remote_ntp_tcp_timeout);
}

#define CONFIG_FILE "/etc/ntp.cfg"

int main(int argc, char **argv) {
	// print_monitor();
	DEBUG("before load conf");
	print_conf();

	load_conf(CONFIG_FILE);
	DEBUG("after load conf");
	print_conf();

	return 0;
}