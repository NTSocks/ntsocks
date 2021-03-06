#include "ntb_monitor.h"
#include "config.h"
#include "nt_log.h"
#include <unistd.h>
#include <string.h>

#define MAX_BUF_LEN 1024
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 256
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

struct ntm_config NTM_CONFIG = {
	/* set default configuration */
	.remote_ntm_tcp_timewait = 0,
	.remote_ntm_tcp_timeout = 1000,
	.listen_ip = NTM_LISTEN_IP,
	.listen_port = NTM_LISTEN_PORT,
	.ipaddr_len = sizeof(NTM_LISTEN_IP),
	.max_concurrency = 65536,
	.max_port = 65536,
	.nt_max_conn_num = 65536,
	.nt_max_port_num = 65536

};

ntm_manager_t ntm_mgr = NULL;

static int trim(char s[])
{
	int n;
	for (n = strlen(s) - 1; n >= 0; n--)
	{
		if (s[n] != ' ' && s[n] != '\t' && s[n] != '\n')
			break;
		s[n + 1] = '\0';
	}
	return n;
}

int load_conf(const char *fname)
{
	DEBUG("load ntb monitor configuration from conf file (%s)", fname);
	FILE *file = fopen(fname, "r");
	if (file == NULL)
	{
		printf("[Error]open %s failed.\n", fname);
		return -1;
	}

	char buf[MAX_BUF_LEN];
	int text_comment = 0;
	while (fgets(buf, MAX_BUF_LEN, file) != NULL)
	{
		trim(buf);
		// to skip text comment with flags /* ... */
		if (buf[0] != '#' && (buf[0] != '/' || buf[1] != '/'))
		{
			if (strstr(buf, "/*") != NULL)
			{
				text_comment = 1;
				continue;
			}
			else if (strstr(buf, "*/") != NULL)
			{
				text_comment = 0;
				continue;
			}
		}
		if (text_comment == 1)
		{
			continue;
		}

		int buf_len = strlen(buf);
		// ignore and skip the line with first chracter '#', '=' or '/'
		if (buf_len <= 1 || buf[0] == '#' || buf[0] == '=' || buf[0] == '/')
		{
			continue;
		}
		buf[buf_len - 1] = '\0';

		char _paramk[MAX_KEY_LEN] = {0}, _paramv[MAX_VAL_LEN] = {0};
		int _kv = 0, _klen = 0, _vlen = 0;
		int i = 0;
		for (i = 0; i < buf_len; ++i)
		{
			if (buf[i] == ' ')
				continue;
			// scan param key name
			if (_kv == 0 && buf[i] != '=')
			{
				if (_klen >= MAX_KEY_LEN)
					break;
				_paramk[_klen++] = buf[i];
				continue;
			}
			else if (buf[i] == '=')
			{
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

		DEBUG("ntb monitor configuration %s=%s", _paramk, _paramv);
		if (strcmp(_paramk, "listen_ip") == 0)
		{
			NTM_CONFIG.ipaddr_len = _vlen;
			memcpy(NTM_CONFIG.listen_ip, _paramv, _vlen);
		}
		else if (strcmp(_paramk, "listen_port") == 0)
		{
			NTM_CONFIG.listen_port = atoi(_paramv);
		}
		else if (strcmp(_paramk, "remote_ntm_tcp_timeout") == 0)
		{
			NTM_CONFIG.remote_ntm_tcp_timeout = atoi(_paramv);
		}
		else if (strcmp(_paramk, "remote_ntm_tcp_timewait") == 0)
		{
			NTM_CONFIG.remote_ntm_tcp_timewait = atoi(_paramv);
		}
		else if (strcmp(_paramk, "max_concurrency") == 0)
		{
			NTM_CONFIG.max_concurrency = atoi(_paramv);
		}
		else if (strcmp(_paramk, "max_port") == 0)
		{
			NTM_CONFIG.max_port = atoi(_paramv);
		}
		else
		{
			return 1;
		}
	}
	return 0;
}

void free_conf()
{
}

void print_conf()
{
	printf("ntm configuration: remote_ntm_tcp_timewait=%d, "
		   "remote_ntm_tcp_timeout=%d;\n ",
		   NTM_CONFIG.remote_ntm_tcp_timewait,
		   NTM_CONFIG.remote_ntm_tcp_timeout);

	printf("listen_ip=%s, listen_port=%d;\n",
		   NTM_CONFIG.listen_ip,
		   NTM_CONFIG.listen_port);

	printf("max_concurrency=%d, max_port=%d.\n",
		   NTM_CONFIG.max_concurrency,
		   NTM_CONFIG.max_port);
}
