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
#include "ntb_proxy.h"
#include "ntp_config.h"
#include "ntlink_parser.h"
#include "ntb_mem.h"
#include "ntp_shm.h"

#define XEON_LINK_STATUS_OFFSET 0x01a2

static uint16_t dev_id;

struct ntb_link *ntb_link;
struct rte_ring *send_ring;
ntm_shm_context_t ntm_ntp_ring, ntp_ntm_ring;

unsigned lcore_id;

static int
ntb_send_thread(__attribute__((unused)) void *arg)
{
	struct ntp_ring_list_node *move_node = ntb_link->ring_head;
	struct ntp_ring_list_node *next_node = NULL;
	while (1)
	{
		next_node = move_node->next_node;
		if (next_node == ntb_link->ring_head)
		{
			move_node = next_node;
			continue;
		}
		if (next_node->conn->state != READY_CONN)
		{
			if (next_node->conn->state == CLOSE_SERVER)
			{
				ntb_send_data(ntb_link->sublink[0], next_node->conn->send_ring);
			}
			ntp_shm_close(next_node->conn->send_ring);
			ntp_shm_close(next_node->conn->recv_ring);
			Remove(ntb_link->map, next_node->conn->name);
			move_node->next_node = next_node->next_node;
			free(next_node);
			//下面返回控制消息给ntp-ntm
			continue;
		}
		ntb_send_data(ntb_link->sublink[0], move_node->conn->send_ring);
		move_node = next_node;
	}
	return 0;
}
static int
ntb_receive_thread(__attribute__((unused)) void *arg)
{
	ntb_receive(ntb_link->sublink[0], ntb_link);
	return 0;
}

static int
ntb_ctrl_receive_thread(__attribute__((unused)) void *arg)
{
	while (1)
	{
		__asm__("mfence");
	}
	return 0;
}

static int
ntm_ntp_receive_thread(__attribute__((unused)) void *arg)
{
	while (1)
	{
		__asm__("mfence");
	}
	return 0;
}

int lcore_ntb_daemon(__attribute__((unused)) void *arg)
{
	int ret, i;
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
	if (ntm_shm_accept(ntm_ntp_ring, ntm_ntp_name, sizeof(ntm_ntp_name)) != 0)
	{
		DEBUG("create ntm_ntp_ring failed\n");
	}
	ntp_ntm_ring = ntm_shm();
	char *ntp_ntm_name = "/ntp-ntm-ring";
	if (ntm_shm_accept(ntp_ntm_ring, ntp_ntm_name, sizeof(ntp_ntm_name)) != 0)
	{
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
