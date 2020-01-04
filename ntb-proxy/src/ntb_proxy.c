/*
 * <p>Title: ntb_proxy.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7
 * @date Nov 23, 2019 
 * @version 1.0
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

#include "nt_log.h"
#include "ntm_shm.h"
#include "ntb_proxy.h"
#include "ntp_config.h"
#include "ntlink_parser.h"
#include "ntb_mem.h"

#define XEON_LINK_STATUS_OFFSET		0x01a2

static uint16_t dev_id;

static const char *_SEND_MSG_POOL = "RPI_MSG_POOL";
static const char *_RECV_MSG_POOL = "SEC_MSG_POOL";
static const char *_SEND_QUE_0 = "PRI_2_SEC_0";

struct rte_ring *send_ring;
ntm_shm_context_t ntm_ntp_ring, ntp_ntm_ring;

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
	struct ntb_custom_message *msg;
	int msg_len;
	int msg_type;
	while (1)
	{
		__asm__("mfence");
		msg = (struct ntb_custom_message *)(r->start_addr + (r->cur_serial * MAX_NTB_msg_LEN));
		msg_len = msg->header.msg_len;
		if (msg_len == 0)
		{
			continue;
		}
		msg_type = ntb_prot_header_parser(sublink, msg);
		if (msg_type == DATA_TYPE)
		{
			ntb_receive(sublink, recv_message_pool);
		}
	}
	return 0;
}

static int
daemon_ctrl_receive_thread(__attribute__((unused)) void *arg)
{
	struct ntb_ring *r = sublink->local_ring;
	struct ntb_custom_message *msg;
	int msg_len;
	int msg_type;
	while (1)
	{
		__asm__("mfence");
		msg = (struct ntb_custom_message *)(r->start_addr + (r->cur_serial * MAX_NTB_msg_LEN));
		msg_len = msg->header.msg_len;
		if (msg_len == 0)
		{
			continue;
		}
		msg_type = ntb_prot_header_parser(sublink, msg);
		if (msg_type == DATA_TYPE)
		{
			ntb_receive(sublink, recv_message_pool);
		}
	}
	return 0;
}

int
lcore_ntb_daemon(__attribute__((unused)) void *arg)
{
	int ret, i;
	struct ntb_link *ntb_link;
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

	ntb_link = ntb_start(dev_id);
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
	//创建名为ntm-ntp以及ntp-ntm的消息队列
	ntm_ntp_ring = ntm_shm();
	char *ntm_ntp_name = "/ntm-ntp-ring";
	if( ntm_shm_accept(ntm_ntp_ring, ntm_ntp_name, sizeof(ntm_ntp_name)) != 0){
		DEBUG("create ntm_ntp_ring failed\n");
	}
	ntp_ntm_ring = ntm_shm();
	char *ntp_ntm_name = "/ntp-ntm-ring";
	if( ntm_shm_accept(ntp_ntm_ring, ntp_ntm_name, sizeof(ntp_ntm_name)) != 0){
		DEBUG("create ntm_ntp_ring failed\n");
	}
	RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
		if (lcore_id == 8)
		{
			rte_eal_remote_launch(daemon_receive_thread, NULL, lcore_id);
		}
	}
	daemon_send_thread(arg);
	return 0;
}
