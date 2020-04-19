/*
 * <p>Title: main.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7
 * @date Nov 12, 2019 
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

#include "ntb_mw.h"
#include "ntp_func.h"
#include "ntb.h"
#include "ntb_hw_intel.h"
#include "ntm_msg.h"
#include "ntp2nts_shm.h"
#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"
#include "nt_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define XEON_LINK_STATUS_OFFSET 0x01a2
#define RTE_RAWDEV_MAX_DEVS 64
#define NTB_DRV_NAME_LEN 7
#define EXIT_FAILURE 1

#define CREATE_CONN 1

static uint16_t dev_id;

struct ntb_link_custom *ntb_link;

unsigned lcore_id;

static int
ntb_send_thread(__attribute__((unused)) void *arg)
{
	assert(ntb_link);
	ntp_send_list_node *pre_node = ntb_link->send_list.ring_head;
	ntp_send_list_node *curr_node = NULL;
	uint64_t counter = 0;
	while (1)
	{
		curr_node = pre_node->next_node;
		if (curr_node == ntb_link->send_list.ring_head)	// indicate non-existing ntb connection when head node in ntb-conn list is EMPTY.
		{
			pre_node = curr_node;
			continue;
		}
		// when the ntb connection state is `PASSIVE CLOSE` or `ACTIVE CLOSE`, remove/clear current ntb connection
		if (curr_node->conn->state == PASSIVE_CLOSE || curr_node->conn->state == ACTIVE_CLOSE)
		{
			DEBUG("conn close,remove and free node");
			// conn->state 不为READY，队列均已Close，移除map、list并free就可
			Remove(ntb_link->port2conn, &curr_node->conn->conn_id);		// remove ntb conn from hash map
			// destory_conn_ack(ntb_link, next_node->conn->name);
			pre_node->next_node = curr_node->next_node;	// remove ntb conn from traseval list
			if(curr_node == ntb_link->send_list.ring_tail){
				ntb_link->send_list.ring_tail = pre_node;
			}
			ntp_shm_ntm_close(curr_node->conn->nts_recv_ring);
			ntp_shm_destroy(curr_node->conn->nts_recv_ring);
			ntp_shm_ntm_close(curr_node->conn->nts_send_ring);
			ntp_shm_destroy(curr_node->conn->nts_send_ring);
			// ntb_conn *conn;
			free(curr_node->conn);
			free(curr_node);
			continue;
		}
		// INFO("send conn name = %s",curr_node->conn->name);
		// counter: is used to DEBUG/test
		// DEBUG("send buff conn_id = %d,shm name = %s",curr_node->conn->conn_id,curr_node->conn->nts_send_ring->shm_addr);
		ntp_send_buff_data(ntb_link->data_link, curr_node->conn->nts_send_ring,curr_node->conn);
		pre_node = curr_node;	// move the current point to next ntb conn
	}
	return 0;
}

static int
ntb_receive_thread(__attribute__((unused)) void *arg)
{
	ntp_receive_data_to_buff(ntb_link->data_link, ntb_link);
	return 0;
}

static int
ntb_ctrl_receive_thread(__attribute__((unused)) void *arg)
{
	ntp_ctrl_msg_receive(ntb_link);
	return 0;
}

static int
ntm_ntp_receive_thread(__attribute__((unused)) void *arg)
{
	ntm_ntp_shm_context_t recv_shm = ntb_link->ntm_ntp;
	ntm_ntp_msg recv_msg;

	//test mode
	// recv_msg.dst_port = 80;
	// recv_msg.src_port = 80;
	// recv_msg.msg_type = 1;
	// recv_msg.dst_ip = 1000020;
	// recv_msg.src_ip = 1000020;
	// ntp_create_conn_handler(ntb_link, &recv_msg);
	//test mode end

	while (1)
	{
		if (ntm_ntp_shm_recv(recv_shm, &recv_msg) == -1)
		{
			continue;
		}
		else
		{
			DEBUG("receive ntm_ntp_msg, create ntb_conn");
			if (recv_msg.msg_type == CREATE_CONN)
			{
				ntp_create_conn_handler(ntb_link, &recv_msg);
			}
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret, i;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization.\n");
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
	DEBUG("mem addr == %ld ,len == %ld", ntb_link->hw->pci_dev->mem_resource[2].phys_addr, ntb_link->hw->pci_dev->mem_resource[2].len);
	uint16_t reg_val;
	if (ntb_link->hw == NULL)
	{
		ERR("Invalid device.");
		return -1;
	}

	ret = rte_pci_read_config(ntb_link->hw->pci_dev, &reg_val,
							  sizeof(reg_val), XEON_LINK_STATUS_OFFSET);
	if (ret < 0)
	{
		ERR("Unable to get link status.");
		return -1;
	}
	ntb_link->hw->link_status = NTB_LNK_STA_ACTIVE(reg_val);
	if (ntb_link->hw->link_status)
	{
		ntb_link->hw->link_speed = NTB_LNK_STA_SPEED(reg_val);
		ntb_link->hw->link_width = NTB_LNK_STA_WIDTH(reg_val);
	}
	RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
		if (lcore_id == 8)
		{
			rte_eal_remote_launch(ntb_receive_thread, NULL, lcore_id);
		}
		if (lcore_id == 9)
		{
			rte_eal_remote_launch(ntb_send_thread, NULL, lcore_id);
		}
		if (lcore_id == 10)
		{
			rte_eal_remote_launch(ntb_ctrl_receive_thread, NULL, lcore_id);
		}
		if (lcore_id == 11)
		{
			rte_eal_remote_launch(ntm_ntp_receive_thread, NULL, lcore_id);
		}
	}
	rte_eal_mp_wait_lcore();
	return 0;
}
