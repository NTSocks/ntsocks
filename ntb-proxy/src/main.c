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
#include <sys/types.h>
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

#include <pthread.h>
#include <time.h>
#include <sched.h>

#include "ntb_mw.h"
#include "ntp_func.h"
#include "ntb.h"
#include "ntb_hw_intel.h"
#include "ntm_msg.h"
#include "ntp2nts_shm.h"
#include "ntm_ntp_shm.h"
#include "ntp_ntm_shm.h"
#include "config.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define NTP_CONFIG_FILE "/etc/ntp.cfg"

#define XEON_LINK_STATUS_OFFSET 0x01a2
#define RTE_RAWDEV_MAX_DEVS 64
#define NTB_DRV_NAME_LEN 7
#define EXIT_FAILURE 1

#define CREATE_CONN 1
#define DEFAULT_SLEEP_US 100
#define SPIN_THRESHOLD 100

static uint16_t dev_id;

struct ntb_link_custom *ntb_link;

unsigned lcore_id;

// define the CPU cores set for ntb_partitions as an array
// 1. the array index indicates the ntb_partition id
// 2. the array value indicates the assigned CPU cores lcore_id;
static int cpu_cores[8] = {8, 9, 10, 11, 12, 13, 14, 15};

static int
ntb_send_thread(__attribute__((unused)) void *arg)
{
	assert(ntb_link);
	assert(arg);

	ntb_partition_t partition;
	partition = (ntb_partition_t)arg;
	DEBUG("enter partition %d", partition->id);

	ntp_send_list_node *pre_node = partition->send_list.ring_head;
	ntp_send_list_node *curr_node = NULL;
	uint64_t counter = 0;
	while (!ntb_link->is_stop)
	{
		curr_node = pre_node->next_node;
		// indicate non-existing ntb connection
		//	when head node in ntb-conn list is EMPTY.
		if (curr_node == partition->send_list.ring_head)
		{
			pre_node = curr_node;
			continue;
		}
		// when the ntb connection state is `PASSIVE CLOSE` or `ACTIVE CLOSE`,
		//	remove/clear current ntb connection
		if (curr_node->conn->state == PASSIVE_CLOSE ||
			curr_node->conn->state == ACTIVE_CLOSE)
		{
			DEBUG("conn close,remove and free node");

			// remove ntb_partition
			curr_node->conn->partition_id = -1;
			curr_node->conn->partition = NULL;

			// conn->state 不为READY，队列均已Close，移除map、list并free就可
			//	remove ntb conn from hash map
			Remove(ntb_link->port2conn, &curr_node->conn->conn_id);

			// remove ntb conn from traseval list
			pre_node->next_node = curr_node->next_node;
			if (curr_node == partition->send_list.ring_tail)
			{
				partition->send_list.ring_tail = pre_node;
			}

			ntp_shm_nts_close(curr_node->conn->nts_recv_ring);
			ntp_shm_destroy(curr_node->conn->nts_recv_ring);
			ntp_shm_nts_close(curr_node->conn->nts_send_ring);
			ntp_shm_destroy(curr_node->conn->nts_send_ring);

			free(curr_node->conn);
			free(curr_node);

			continue;
		}

		ntp_send_buff_data(partition->data_link, partition,
						   curr_node->conn->nts_send_ring, curr_node->conn);
		pre_node = curr_node; // move the current point to next ntb conn
	}

	return 0;
}

static int
ntb_receive_thread(__attribute__((unused)) void *arg)
{
	assert(ntb_link);
	assert(arg);

	ntb_partition_t partition;
	partition = (ntb_partition_t)arg;
	DEBUG("enter partition %d", partition->id);

	ntp_receive_data_to_buff(
		partition->data_link, ntb_link, partition);

	return 0;
}

static void *
ntb_ctrl_receive_thread(__attribute__((unused)) void *arg)
{
	ntp_ctrl_msg_receive(ntb_link);

	return NULL;
}

static void *
ntm_ntp_receive_thread(__attribute__((unused)) void *arg)
{
	ntm_ntp_shm_context_t recv_shm = ntb_link->ntm_ntp;
	ntm_ntp_msg recv_msg;
	uint64_t loop_spin_cnt = 0; // default sleep, when loop_spin_cnt > 10

	while (!ntb_link->is_stop)
	{
		if (UNLIKELY(ntm_ntp_shm_recv(recv_shm, &recv_msg) == -1))
		{
			sched_yield();
			loop_spin_cnt++;
			if (loop_spin_cnt == SPIN_THRESHOLD)
			{
				loop_spin_cnt = 0;
				usleep(DEFAULT_SLEEP_US);
			}

			continue;
		}
		else
		{
			DEBUG("receive ntm_ntp_msg, create ntb_conn");
			loop_spin_cnt = 0;
			if (recv_msg.msg_type == CREATE_CONN)
			{
				ntp_create_conn_handler(ntb_link, &recv_msg);
			}
		}
	}

	return NULL;
}

// for quit signal
static volatile bool s_signal_quit = false;
static volatile int s_signum = -1;

static void before_exit(void)
{
	INFO("destroy all ntm resources when on exit.");
	if (!s_signal_quit) 
	{
		s_signal_quit = true;
		ntb_destroy(ntb_link);

		if (s_signum != -1)
		{
			kill(getpid(), s_signum);
		}
	}
}

static void crash_handler(int signum)
{
	printf("\n[Crash]: Signal %d received, preparing to exit...\n", signum);
	s_signum = signum;
	exit(-1);
}

static void signal_exit_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) 
	{
		printf("\nSignal %d received, preparing to exit...\n", signum);
		s_signum = signum;
	}
	exit(0);
}


static void *
ntp_epoll_listen_thread(__attribute__((unused)) void *arg)
{
	assert(ntb_link);

	epoll_sem_shm_ctx_t ep_recv_ctx = ntb_link->ntp_ep_recv_ctx;
	epoll_sem_shm_ctx_t ep_send_ctx = ntb_link->ntp_ep_send_ctx;

	epoll_msg req_msg;
	int rc;

	while (!ntb_link->is_stop)
	{
		rc = epoll_sem_shm_recv(ep_recv_ctx, &req_msg);
		if (LIKELY(rc == 0))
		{
			DEBUG("receive one epoll_msg from ntm");
			rc = ntp_handle_epoll_msg(ntb_link, &req_msg);

			if (rc != 0)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	INFO("ntp_epoll_listen_thread exit!");
	return NULL;
}

int main(int argc, char **argv)
{
	int ret, i;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
	{
		rte_exit(EXIT_FAILURE, "Error with EAL initialization.\n");
	}

	/* Find 1st ntb rawdev. */
	for (i = 0; i < RTE_RAWDEV_MAX_DEVS; i++)
	{
		if (rte_rawdevs[i].driver_name &&
			(strncmp(rte_rawdevs[i].driver_name, "raw_ntb", NTB_DRV_NAME_LEN) == 0) &&
			(rte_rawdevs[i].attached == 1))
		{
			break;
		}
	}

	if (i == RTE_RAWDEV_MAX_DEVS)
	{
		rte_exit(EXIT_FAILURE, "Cannot find any ntb device.\n");
	}

	// register exit event processing
	atexit(before_exit);
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);

	signal(SIGBUS, crash_handler);  // 总线错误
	signal(SIGSEGV, crash_handler); // SIGSEGV，非法内存访问
	signal(SIGFPE, crash_handler);  // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等
	signal(SIGABRT, crash_handler); // SIGABRT，由调用abort函数产生，进程非正常退出

	dev_id = i;

	/** Load the NTP config file */
	char *conf_file;

	if (!conf_file)
	{
		conf_file = NTP_CONFIG_FILE;
	}

	/** load the customized NTP parameters */
	if (load_conf(conf_file) == -1)
	{
		rte_exit(EXIT_FAILURE, "Cannot load NTP configuration file.\n");
	}

	print_conf();

	NTP_CONFIG.datapacket_payload_size =
		NTP_CONFIG.data_packet_size - NTPACKET_HEADER_LEN;

	ntb_link = ntb_start(dev_id);
	if (!ntb_link) 
	{
		fprintf(stderr, "\n*********** NTB Link start failed ***********\n");
		rte_exit(EXIT_FAILURE, "NTB Link start failed.\n");
	}

	DEBUG("mem addr == %ld ,len == %ld",
		  ntb_link->hw->pci_dev->mem_resource[2].phys_addr,
		  ntb_link->hw->pci_dev->mem_resource[2].len);

	if (ntb_link->hw == NULL)
	{
		ERR("Invalid device.");
		rte_exit(EXIT_FAILURE, "Invalid NTB device.\n");
	}

	uint16_t reg_val;
	ret = rte_pci_read_config(ntb_link->hw->pci_dev, &reg_val,
							  sizeof(reg_val), XEON_LINK_STATUS_OFFSET);
	if (ret < 0)
	{
		ERR("Unable to get NTB link status.");
        rte_exit(EXIT_FAILURE, "Unable to get NTB link status.\n");
	}

	ntb_link->hw->link_status = NTB_LNK_STA_ACTIVE(reg_val);
	if (ntb_link->hw->link_status)
	{
		ntb_link->hw->link_speed = NTB_LNK_STA_SPEED(reg_val);
		ntb_link->hw->link_width = NTB_LNK_STA_WIDTH(reg_val);
	}

	pthread_create(&ntb_link->ntm_ntp_listener,
				   NULL, ntm_ntp_receive_thread, NULL);
	pthread_create(&ntb_link->ctrl_recv_thr,
				   NULL, ntb_ctrl_receive_thread, NULL);
	pthread_create(&ntb_link->epoll_listen_thr,
				   NULL, ntp_epoll_listen_thread, NULL);

	RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
		for (int i = 0; i < ntb_link->num_partition; i++)
		{
			int j = i * 2;
			if (lcore_id == cpu_cores[j])
			{
				rte_eal_remote_launch(ntb_receive_thread,
									  (void *)&ntb_link->partitions[i], lcore_id);
			}
			if (lcore_id == cpu_cores[j + 1])
			{
				rte_eal_remote_launch(ntb_send_thread,
									  (void *)&ntb_link->partitions[i], lcore_id);
			}
		}
	}
	rte_eal_mp_wait_lcore();

	return 0;
}
