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

static const char *_SEND_MSG_POOL = "RPI_MSG_POOL";
static const char *_RECV_MSG_POOL = "SEC_MSG_POOL";
static const char *_SEND_QUE_0 = "PRI_2_SEC_0";

struct rte_ring *send_ring;
struct rte_mempool *send_message_pool, *recv_message_pool;
struct ntb_custom_sublink *sublink;

const unsigned flags = 0;
const unsigned ring_size = 0x1000;
const unsigned pool_cache = 0;
const unsigned priv_data_sz = 0;
const unsigned pool_size = 0x1000;
const unsigned pool_elt_size = 0x800;
const unsigned app_data_len = 0x800 - MEM_NODE_HEADER_LEB;
const unsigned start_locre = 90;
const int how_many_app = 2;

unsigned lcore_id;

static int
daemon_send_thread(__attribute__((unused)) void *arg)
{
	void *msg;
	while (1)
	{
		//读取send_ring中的指针
		if (rte_ring_dequeue(send_ring, &msg) < 0)
		{
			continue;
		}
		ntb_send(sublink, msg);
		//put 回send pool
		rte_mempool_put(send_message_pool, msg);
	}
	return 0;
}
static int
daemon_receive_thread(__attribute__((unused)) void *arg)
{
	struct ntb_ring *r = sublink->local_ring;
	struct ntb_custom_message *mss;
	int mss_len;
	int mss_type;
	while (1)
	{
		__asm__("mfence");
		mss = (struct ntb_custom_message *)(r->start_addr + (r->cur_serial * MAX_NTB_MSS_LEN));
		mss_len = mss->header.mss_len;
		if (mss_len == 0)
		{
			continue;
		}
		mss_type = ntb_prot_header_parser(sublink, mss);
		if (mss_type == DATA_TYPE)
		{
			ntb_receive(sublink, recv_message_pool);
		}
	}
	return 0;
}

static int
lcore_ntb_daemon(__attribute__((unused)) void *arg)
{
	int ret, i;
	struct ntb_custom_link *ntb_link;
	// void *msg = NULL;
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

	ntb_link = ntb_custom_start(dev_id);
	NTB_LOG(DEBUG, "mem addr == %ld ,len == %ld", ntb_link->hw->pci_dev->mem_resource[2].phys_addr, ntb_link->hw->pci_dev->mem_resource[2].len);
	NTB_LOG(DEBUG, "I'm daemon!");
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
	}
	sublink = &ntb_link->sublink[0];
	//daemon共享的发送ring、接收ring、发送mempool、接收mempool
	send_ring = rte_ring_create(_SEND_QUE_0, ring_size, rte_socket_id(), flags);
	for (int temp = 0; temp < (int)how_many_app; temp++)
	{
		char recv_temp[12] = "SEC_2_PRI_x";
		recv_temp[10] = (char)(temp + 48);
		const char *recv_que = recv_temp;
		rte_ring_create(recv_que, ring_size, rte_socket_id(), flags);
		NTB_LOG(DEBUG, "daemon_recv_ring = %s.", recv_que);
	}
	send_message_pool = rte_mempool_create(_SEND_MSG_POOL, pool_size,
										   pool_elt_size, pool_cache, priv_data_sz,
										   NULL, NULL, NULL, NULL,
										   rte_socket_id(), flags);
	recv_message_pool = rte_mempool_create(_RECV_MSG_POOL, pool_size,
										   pool_elt_size, pool_cache, priv_data_sz,
										   NULL, NULL, NULL, NULL,
										   rte_socket_id(), flags);
	NTB_LOG(DEBUG, "run receive thread.");
	RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
		if (lcore_id == 8)
		{
			rte_eal_remote_launch(daemon_receive_thread, NULL, lcore_id);
		}
	}
	NTB_LOG(DEBUG, "run send thread.");
	daemon_send_thread(arg);
	return 0;
}

static int
lcore_ntb_app(__attribute__((unused)) void *arg)
{
	unsigned my_lcore_id = rte_lcore_id();
	struct rte_ring *recv_ring;
	struct ntb_uuid myuuid;
	myuuid.ntb_port = (uint16_t)(my_lcore_id - start_locre);
	uint64_t prev_tsc, cur_tsc, timer_tsc = 0;
	uint64_t counter = 0;
	uint64_t timer_hz = rte_get_timer_hz();
	send_ring = rte_ring_lookup(_SEND_QUE_0);
	//查找自己的接收队列
	char recv_temp[12] = "SEC_2_PRI_x";
	NTB_LOG(DEBUG, "my_id = %d.", (int)(my_lcore_id - start_locre));
	recv_temp[10] = (char)((int)(my_lcore_id - start_locre) + 48);
	const char *recv_que = recv_temp;
	recv_ring = rte_ring_lookup(recv_que);
	NTB_LOG(DEBUG, "app_recv_ring = %s.", recv_que);
	send_message_pool = rte_mempool_lookup(_SEND_MSG_POOL);
	recv_message_pool = rte_mempool_lookup(_RECV_MSG_POOL);
	NTB_LOG(DEBUG, "I'm app.");
	void *msg = NULL;
	while (1)
	{
		//recv ring中有memnode
		while (rte_ring_dequeue(recv_ring, &msg) == 0)
		{
			//接收recv中的memnode
			rte_mempool_put(recv_message_pool, msg);
			//get memnode后放入send ring
			ntb_app_mempool_get(send_message_pool, &msg, &myuuid);
			*(uint16_t *)((uint8_t *)msg + 2) = app_data_len;
			while (rte_ring_enqueue(send_ring, msg) < 0)
			{
				//入ring 失败，//放回mempool
				NTB_LOG(ERR, "Failed to send message - message discarded.");
				//rte_mempool_put(send_message_pool, msg);
			}
			prev_tsc = rte_rdtsc();
			while (rte_ring_dequeue(recv_ring, &msg) != 0)
			{
				//尚无memnode返回
			}
			cur_tsc = rte_rdtsc();
			counter++;
			timer_tsc += cur_tsc - prev_tsc;
			NTB_LOG(DEBUG, "time_tsc = %ld,app latency = %lg us.", timer_tsc, (double)timer_tsc * US_PER_S / counter / timer_hz);
			// }
			rte_mempool_put(recv_message_pool, msg);
			ntb_app_mempool_get(send_message_pool, &msg, &myuuid);
			while (rte_ring_enqueue(send_ring, msg) < 0)
			{
				//入ring 失败，//放回mempool
				NTB_LOG(ERR, "Failed to send message - message discarded.");
				//rte_mempool_put(send_message_pool, msg);
			}
		}
	}
	return 0;
}

static int
lcore_ntb_app_sr(__attribute__((unused)) void *arg)
{
	unsigned my_lcore_id = rte_lcore_id();
	struct rte_ring *recv_ring;
	struct ntb_uuid myuuid;
	myuuid.ntb_port = (uint16_t)(my_lcore_id - start_locre);
	send_ring = rte_ring_lookup(_SEND_QUE_0);
	char recv_temp[12] = "SEC_2_PRI_x";
	NTB_LOG(DEBUG, "my_id = %d.", (int)(my_lcore_id - start_locre));
	recv_temp[10] = (char)((int)(my_lcore_id - start_locre) + 48);
	const char *recv_que = recv_temp;
	recv_ring = rte_ring_lookup(recv_que);
	NTB_LOG(DEBUG, "app_recv_ring = %s.", recv_que);
	send_message_pool = rte_mempool_lookup(_SEND_MSG_POOL);
	recv_message_pool = rte_mempool_lookup(_RECV_MSG_POOL);
	NTB_LOG(DEBUG, "I'm app.");
	void *msg = NULL;
	ntb_app_mempool_get(send_message_pool, &msg, &myuuid);
	while (rte_ring_enqueue(send_ring, msg) < 0)
	{
		//入ring 失败，//放回mempool
		NTB_LOG(ERR, "Failed to send message - message discarded.");
		//rte_mempool_put(send_message_pool, msg);
	}
	while (1)
	{
		while (rte_ring_dequeue(recv_ring, &msg) == 0)
		{
			//接收recv中的memnode
			rte_mempool_put(recv_message_pool, msg);
			//get memnode后放入send ring
			ntb_app_mempool_get(send_message_pool, &msg, &myuuid);
			*(uint16_t *)((uint8_t *)msg + 2) = app_data_len;
			while (rte_ring_enqueue(send_ring, msg) < 0)
			{
				//入ring 失败，//放回mempool
				NTB_LOG(ERR, "Failed to send message - message discarded.");
				//rte_mempool_put(send_message_pool, msg);
			}
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization.\n");
	if (rte_eal_process_type() == RTE_PROC_PRIMARY)
	{
		RTE_LCORE_FOREACH_SLAVE(lcore_id)
		{
			if (lcore_id == 7)
			{
				rte_eal_remote_launch(lcore_ntb_daemon, NULL, lcore_id);
			}
		}
	}
	else
	{
		if (argv[2])
		{
			RTE_LCORE_FOREACH_SLAVE(lcore_id)
			{
				if (lcore_id < start_locre + how_many_app && lcore_id >= start_locre)
				{
					rte_eal_remote_launch(lcore_ntb_app, NULL, lcore_id);
				}
			}
		}
		else
		{
			RTE_LCORE_FOREACH_SLAVE(lcore_id)
			{
				if (lcore_id < start_locre + how_many_app && lcore_id >= start_locre)
				{
					rte_eal_remote_launch(lcore_ntb_app_sr, NULL, lcore_id);
				}
			}
		}
	}
	rte_eal_mp_wait_lcore();
	return 0;
}
