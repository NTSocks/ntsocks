/*
 * <p>Title: config.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Wenxiong Zou
 * @date Dec 16, 2019 
 * @version 1.0
 */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ntb.h"
#include "config.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define MAX_BUF_LEN 1024
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 256
#define DEFAULT_NUM_PARTITION 2
#define DEFAULT_NTPACKET_SIZE 7
#define DEFAULT_CTRL_PACKET_SIZE 16
#define DEFAULT_DATA_RING_SIZE 0x4000000 // 8MB --> 64MB
#define DEFAULT_CTRL_RING_SIZE 0x40000	 // 256KB
#define MAX_NUM_PARTITION 4

struct ntp_config NTP_CONFIG = {
	/* set default configuration */
	.sublink_data_ring_size = 8388608,
	.sublink_ctrl_ring_size = 262144,
	.nts_buff_size = 8388608,
	.bulk_size = 1,
	.num_partition = DEFAULT_NUM_PARTITION,
	.ntb_packetbits_size = DEFAULT_NTPACKET_SIZE,
	.ctrl_packet_size = DEFAULT_CTRL_PACKET_SIZE,
	.data_ringbuffer_size = DEFAULT_DATA_RING_SIZE,
	.ctrl_ringbuffer_size = DEFAULT_CTRL_RING_SIZE
};

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

static int math_log2(int value) {
	int log_v = log(value * 1.0) / log(2.0);
	return log_v;
}

int load_conf(const char *fname)
{
	DEBUG("load ntb proxy configuration from conf file (%s)", fname);
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
		// DEBUG("ntb proxy configuration %s=%s", _paramk, _paramv);
		if (strcmp(_paramk, "sublink_data_ring_size") == 0)
		{
			NTP_CONFIG.sublink_data_ring_size = atoi(_paramv);
		}
		else if (strcmp(_paramk, "sublink_ctrl_ring_size") == 0)
		{
			NTP_CONFIG.sublink_ctrl_ring_size = atoi(_paramv);
		}
		else if (strcmp(_paramk, "nts_buff_size") == 0)
		{
			NTP_CONFIG.nts_buff_size = atoi(_paramv);
		}
		else if (strcmp(_paramk, "bulk_size") == 0)
		{
			NTP_CONFIG.bulk_size = atoi(_paramv);
		}
		else if (strcmp(_paramk, "num_partition") == 0)
		{
			NTP_CONFIG.num_partition = atoi(_paramv);
			NTP_CONFIG.num_partition = NTP_CONFIG.num_partition <= MAX_NUM_PARTITION ? 
											NTP_CONFIG.num_partition : MAX_NUM_PARTITION; 
		}
		else if (strcmp(_paramk, "ntb_packetbits_size") == 0)
		{
			NTP_CONFIG.ntb_packetbits_size = atoi(_paramv);
			NTP_CONFIG.data_packet_size = 1 << NTP_CONFIG.ntb_packetbits_size;
		}
		else if (strcmp(_paramk, "ctrl_packet_size") == 0)
		{
			NTP_CONFIG.ctrl_packet_size = atoi(_paramv);
		}
		else if (strcmp(_paramk, "data_ringbuffer_size") == 0)
		{
			char *end;
			NTP_CONFIG.data_ringbuffer_size = strtoull(_paramv, &end, 16);
		}
		else if (strcmp(_paramk, "ctrl_ringbuffer_size") == 0)
		{
			char *end;
			NTP_CONFIG.ctrl_ringbuffer_size = strtoll(_paramv, &end, 16);
		}
		else if (strcmp(_paramk, "data_packet_size") == 0)
		{
			NTP_CONFIG.data_packet_size = atoi(_paramv);
			NTP_CONFIG.ntb_packetbits_size = math_log2(NTP_CONFIG.data_packet_size);
		}
		else
		{
			fclose(file);
			return -1;
		}
	}
	fclose(file);

	return 0;
}

void free_conf() {
	
}

void print_conf() {
	printf("sublink_data_ring_size = %d\n\
		sublink_ctrl_ring_size = %d\n\
		nts_buff_size = %d\n\
		bulk_size = %d\n\
		num_partition = %d\n\
		ntb_packetbits_size = %d\n\
		data_packet_size = %d\n\
		ctrl_packet_size = %d\n\
		data_ringbuffer_size = %ld\n\
		ctrl_ringbuffer_size = %ld\n\n", 
	   NTP_CONFIG.sublink_data_ring_size, 
	   NTP_CONFIG.sublink_ctrl_ring_size,
	   NTP_CONFIG.nts_buff_size,
	   NTP_CONFIG.bulk_size,
	   NTP_CONFIG.num_partition,
	   NTP_CONFIG.ntb_packetbits_size,
	   NTP_CONFIG.data_packet_size,
	   NTP_CONFIG.ctrl_packet_size,
	   NTP_CONFIG.data_ringbuffer_size,
	   NTP_CONFIG.ctrl_ringbuffer_size);
}
