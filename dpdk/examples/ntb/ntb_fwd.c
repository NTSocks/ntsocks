/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

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
#include <rte_ring.h>
#include <rte_mbuf.h>
#include <rte_cycles.h>

#define NTB_DRV_NAME_LEN 7
#define RUN_TIME_CUSTOM 10

// static uint16_t dev_id;

static const char *_SEND_MSG_POOL = "RPI_MSG_POOL";
static const char *_RECV_MSG_POOL = "SEC_MSG_POOL";
static const char *_SEND_QUE_0 = "PRI_2_SEC_0";

struct rte_ring *send_ring;
struct rte_mempool *send_message_pool, *recv_message_pool;

const unsigned flags = 0;
const unsigned ring_size = 0x1000;
const unsigned pool_cache = 0;
const unsigned priv_data_sz = 0;
const unsigned pool_size = 0x1000;
const unsigned pool_elt_size = 0x2000;
// const unsigned app_data_len = 0x1000 - MEM_NODE_HEADER_LEB;
// const unsigned start_locre = 90;
// const int how_many_app = 1;

unsigned lcore_id;

int main(int argc, char **argv)
{
	int ret;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization.\n");
	send_message_pool = rte_mempool_create(_SEND_MSG_POOL, pool_size,
										   pool_elt_size, pool_cache, priv_data_sz,
										   NULL, NULL, NULL, NULL,
										   rte_socket_id(), flags);
	recv_message_pool = rte_mempool_create(_RECV_MSG_POOL, pool_size,
										   pool_elt_size, pool_cache, priv_data_sz,
										   NULL, NULL, NULL, NULL,
										   rte_socket_id(), flags);
	send_ring = rte_ring_create(_SEND_QUE_0, ring_size, rte_socket_id(), flags);
	while(1){

	}
	return 0;
}
