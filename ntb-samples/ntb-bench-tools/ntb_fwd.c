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

#include "ntb.h"
#include "ntb_hw_intel.h"
#include "ntb_custom.h"
#include "ntb_trans_protocols.h"

#define NTB_DRV_NAME_LEN 7
#define RUN_TIME_CUSTOM 10

static uint16_t dev_id;

static const char *_SEC_MSG_POOL = "SEC_MSG_POOL";
static const char *_PRI_MSG_POOL = "RPI_MSG_POOL";
static const char *_SEC_2_PRI = "SEC_2_PRI";
static const char *_PRI_2_SEC = "PRI_2_SEC";

struct rte_ring *send_ring, *recv_ring;
struct rte_mempool *send_message_pool, *recv_message_pool;

const unsigned flags = 0;
const unsigned ring_size = 4096;
const unsigned pool_size = 1024;
const unsigned pool_cache = 32;
const unsigned priv_data_sz = 0;
const unsigned pool_elt_size = 0x1000;

static int
lcore_ntb_daemon(__attribute__((unused)) void *arg)
{
	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc = 0;
	uint64_t counter = 0;
	int ret, i;
	//unsigned lcore_id;
	struct ntb_custom_link *ntb_link;
	/* Find 1st ntb rawdev. */
	for (i = 0; i < RTE_RAWDEV_MAX_DEVS; i++)
		if (rte_rawdevs[i].driver_name &&
			(strncmp(rte_rawdevs[i].driver_name, "raw_ntb",
					 NTB_DRV_NAME_LEN) == 0) &&
			(rte_rawdevs[i].attached == 1))
			break;

	if (i == RTE_RAWDEV_MAX_DEVS)
		rte_exit(EXIT_FAILURE, "Cannot find any ntb device.\n");

	dev_id = i;

	// argc -= ret;
	// argv += ret;

	ntb_link = ntb_custom_start(dev_id);

	uint64_t timer_period = RUN_TIME_CUSTOM * rte_get_timer_hz();
	printf("timer_period == %'ld \n", timer_period);
	printf("mem addr == %ld ,len == %ld\n", ntb_link->hw->pci_dev->mem_resource[2].phys_addr, ntb_link->hw->pci_dev->mem_resource[2].len);
	printf("I'm daemon!\n");
	uint16_t reg_val;

	if (ntb_link->hw == NULL)
	{
		NTB_LOG(ERR, "Invalid device.");
		return -EINVAL;
	}

	ret = rte_pci_read_config(ntb_link->hw->pci_dev, &reg_val,
							  sizeof(reg_val), XEON_LINK_STATUS_OFFSET);
	if (ret < 0)
	{
		NTB_LOG(ERR, "Unable to get link status.");
		return -EIO;
	}

	ntb_link->hw->link_status = NTB_LNK_STA_ACTIVE(reg_val);
	if (ntb_link->hw->link_status)
	{
		ntb_link->hw->link_speed = NTB_LNK_STA_SPEED(reg_val);
		ntb_link->hw->link_width = NTB_LNK_STA_WIDTH(reg_val);
		printf("link_speed == %d,link_width == %d \n", ntb_link->hw->link_speed, ntb_link->hw->link_width);
	}
	//使用sublink0，端口号555
	struct ntb_custom_sublink *sublink = &ntb_link->sublink[0];
	struct ntb_socket socket = sublink->process_map[555];
	//daemon共享的发送ring、接收ring、发送mempool、接收mempool
	send_ring = rte_ring_create(_PRI_2_SEC, ring_size, rte_socket_id(), flags);
	recv_ring = rte_ring_create(_SEC_2_PRI, ring_size, rte_socket_id(), flags);
	send_message_pool = rte_mempool_create(_PRI_MSG_POOL, pool_size,
										   pool_elt_size, pool_cache, priv_data_sz,
										   NULL, NULL, NULL, NULL,
										   rte_socket_id(), flags);
	recv_message_pool = rte_mempool_create(_SEC_MSG_POOL, pool_size,
										   pool_elt_size, pool_cache, priv_data_sz,
										   NULL, NULL, NULL, NULL,
										   rte_socket_id(), flags);
	printf("start switch!!\n");
	switch (ntb_link->hw->topo)
	{
	case NTB_TOPO_B2B_USD:
		prev_tsc = rte_rdtsc();
		ntb_send_open_link(sublink, 555);
		while (1)
		{
			ntb_mss_dequeue(sublink, socket.rev_buff);
			if (sublink->process_map[555].rev_buff)
			{
				cur_tsc = rte_rdtsc();
				break;
			}
		}
		printf("Round trip tests Lasted : %lg us\n",
			   (double)(cur_tsc-prev_tsc) /
				   rte_get_tsc_hz() * US_PER_S);
		//sublink->process_map[555].send_buff->data_len = NTB_BUFF_SIZE;

		printf("start send mss\n");
		void *msg;
		while (1)
		{
			prev_tsc = rte_rdtsc();
			//读取send_ring中的指针
			if (rte_ring_dequeue(send_ring, &msg) < 0)
			{
				continue;
			}
			sublink->process_map[555].send_buff->buff = (uint8_t *)msg;
			sublink->process_map[555].send_buff->data_len = (uint64_t)pool_elt_size;
			ntb_send(sublink, 555);
			//put 回send pool
			rte_mempool_put(send_message_pool, msg);
			//printf("send 1 block\n");
			counter++;
			cur_tsc = rte_rdtsc();
			diff_tsc = cur_tsc - prev_tsc;
			timer_tsc += diff_tsc;
			//timer_tsc 大于10秒则break
			if (unlikely(timer_tsc >= timer_period))
			{
				break;
			}
			prev_tsc = rte_rdtsc();
		}
		printf("send buff = %d; loop counter = %ld ; BW (bits/s) = %'ld \n", pool_elt_size, counter, (counter * pool_elt_size * 8) / RUN_TIME_CUSTOM);
		break;
	//
	case NTB_TOPO_B2B_DSD:
		while (1)
		{
			socket.rev_buff = sublink->process_map[555].rev_buff;
			ntb_mss_dequeue(sublink, socket.rev_buff);
		}
		break;
	default:
		break;
	}
	return 0;
}

static int
lcore_ntb_app(__attribute__((unused)) void *arg)
{
	unsigned lcore_id = rte_lcore_id();
	send_ring = rte_ring_lookup(_PRI_2_SEC);
	recv_ring = rte_ring_lookup(_SEC_2_PRI);
	send_message_pool = rte_mempool_lookup(_PRI_MSG_POOL);
	recv_message_pool = rte_mempool_lookup(_SEC_MSG_POOL);
	printf("Starting core %u\n", lcore_id);
	printf("I'm app!\n");
	while (1)
	{
		void *msg = NULL;
		//send ring
		if (rte_mempool_get(send_message_pool, &msg) < 0)
		{
			printf("Failed to get message buffer\n");
			continue;
		}
		if (rte_ring_enqueue(send_ring, msg) < 0)
		{
			//入ring 失败，放回mempool
			printf("Failed to send message - message discarded\n");
			rte_mempool_put(send_message_pool, msg);
		}
		//recv ring
		while (rte_ring_dequeue(recv_ring, &msg) == 0)
		{
			//接收recv中的memblock
			rte_mempool_put(recv_message_pool, msg);
		}
		//printf("core %u: Received '%s'\n", lcore_id, (char *)msg);
	}

	return 0;
}

int main(int argc, char **argv)
{
	//uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc = 0;
	//uint64_t counter = 0;
	int ret;
	unsigned lcore_id;
	//struct ntb_custom_link *ntb_link;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization.\n");
	if (rte_eal_process_type() == RTE_PROC_PRIMARY)
	{
		rte_eal_mp_remote_launch(lcore_ntb_daemon, NULL, CALL_MASTER);
		RTE_LCORE_FOREACH_SLAVE(lcore_id)
		{
			if (rte_eal_wait_lcore(lcore_id) < 0)
			{
				ret = -1;
				break;
			}
		}
	}
	else
	{
		rte_eal_mp_remote_launch(lcore_ntb_app, NULL, CALL_MASTER);
		RTE_LCORE_FOREACH_SLAVE(lcore_id)
		{
			if (rte_eal_wait_lcore(lcore_id) < 0)
			{
				ret = -1;
				break;
			}
		}
	}

	return 0;
}
