#include <stdint.h>
#include <errno.h>

#include <rte_io.h>
#include <rte_eal.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_rawdev.h>
#include <rte_rawdev_pmd.h>
#include <rte_memcpy.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_debug.h>
#include <rte_dev.h>
#include <rte_kvargs.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_lcore.h>
#include <rte_bus.h>
#include <rte_bus_vdev.h>
#include <rte_memzone.h>
#include <rte_mempool.h>
#include <rte_rwlock.h>
#include "ntb_mem.h"
#include "ntb.h"
#include "ntb_hw_intel.h"
#include "config.h"
#include "ntlink_parser.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

static char **str_split(char *a_str, const char a_delim)
{
	char **result = 0;
	size_t count = 0;
	char *tmp = a_str;
	char *last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	/* Count how many elements will be extracted. */
	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			count++;
			last_comma = tmp;
		}
		tmp++;
	}

	/* Add space for trailing token. */
	count += last_comma < (a_str + strlen(a_str) - 1);
	/* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
	count++;
	result = malloc(sizeof(char *) * count);
	if (result)
	{
		size_t idx = 0;
		char *token = strtok(a_str, delim);

		while (token)
		{
			assert(idx < count);
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		assert(idx == count - 1);
		*(result + idx) = 0;
	}
	return result;
}

static int parser_ring_name(char *ring_name, uint16_t *src_port, uint16_t *dst_port)
{
	DEBUG("parser_ring_name");
	char name_copy[14];
	strcpy(name_copy, ring_name);
	char **results = str_split(name_copy, '-');
	if (results)
	{
		*src_port = atoi(*results);
		*dst_port = atoi(*(results + 1));
		free(results);
		return 0;
	}
	else
	{
		ERR("parser_ring_name err");
	}
	return -1;
}



