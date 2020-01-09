#include <stdint.h>
#include <errno.h>

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

#include "ntm_shm.h"
#include "ntp_shm.h"
#include "ntp_config.h"
#include "ntlink_parser.h"
#include "ntb_mem.h"
#include "ntb_proxy.h"
#include "nt_log.h"

char **str_split(char *a_str, const char a_delim)
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
		DEBUG("parser_ring_name err");
	}
	return -1;
}

int add_conn_to_ntlink(struct ntb_link *link, ntb_conn *conn)
{
	struct ntp_ring_list_node *ring_node = malloc(sizeof(*ntp_ring_list_node));
	ring_node->conn = conn;
	ring_node->next_node = link->ring_head;
	link->ring_tail->next_node = ring_node;
	return 0;
}

static int ntb_trans_cum_ptr(struct ntb_sublink *sublink)
{
	*sublink->remote_cum_ptr = sublink->local_ring->cur_serial;
	return 0;
}

int ntb_mem_header_parser(struct ntb_sublink *sublink, struct ntb_data_msg *msg)
{
	uint16_t msg_type = msg->header.msg_len;
	if (msg_type & 1 << 15)
	{
		ntb_trans_cum_ptr(sublink);
	}
	msg_type >> 12;
	msg_type &= 0x07;
	return msg_type;
}

static int ntb_msg_add_header(struct ntb_data_msg *msg, uint16_t src_port, uint16_t dst_port, int payload_len, int msg_type)
{
	msg->header.src_port = src_port;
	msg->header.dst_port = dst_port;
	// msg->header.msg_type = DATA_TYPE;
	msg->header.msg_len = NTB_HEADER_LEN + payload_len;
	//End of Memnode
	if (msg_type == SINGLE_PKG)
	{
		msg->header.msg_len |= 1 << 12;
	}
	else if (msg_type == MULTI_PKG)
	{
		msg->header.msg_len |= 1 << 13;
	}
	else if (msg_type == ENF_MULTI)
	{
		msg->header.msg_len |= 3 << 12;
	}
	else if (msg_type == DETECT_PKG)
	{
	}
	else if (msg_type == FIN_PKG)
	{
		msg->header.msg_lFINen |= 1 << 14;
	}
	else
	{
		ERR("Error msg_type");
	}
	return 0;
}

static int ntb_data_enqueue(struct ntb_sublink *sublink, uint8_t *msg, int data_len)
{
	struct ntb_ring *r = sublink->remote_ring;
	uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
	//looping send
	while (next_serial == *sublink->local_cum_ptr)
	{
	}
	uint8_t *ptr = r->start_addr + r->cur_serial * NTB_DATA_MSG_TL;
	rte_memcpy(ptr, msg, data_len);
	r->cur_serial = next_serial;
	return 0;
}

static int ntb_msg_enqueue(struct ntb_sublink *sublink, struct ntb_data_msg *msg)
{
	//msg_len 为包含头部的总长度，add_header时计算
	struct ntb_ring *r = sublink->remote_ring;
	uint16_t msg_len = msg->header.msg_len;
	msg_len &= 0x0fff;
	uint64_t next_serial = (r->cur_serial + 1) % (r->capacity);
	//looping send
	while (next_serial == *sublink->local_cum_ptr)
	{
	}
	if ((next_serial - *sublink->local_cum_ptr) & 0x3ff == 0)
	{
		//PSH
		msg->header.msg_len |= 1 << 15;
	}
	uint16_t msg_type = msg->header.msg_len & 0x7000;
	msg_type = msg_type >> 12;
	if (msg_type == MULTI_PKG)
	{
		//判断加上data包后，有没有超过cum ptr trans line
		if ((next_serial + msg_len / NTB_DATA_MSG_TL) & 0xfc00 != next_serial & 0xfc00)
		{
			msg->header.msg_len |= 1 << 15;
		}
	}
	uint8_t *ptr = r->start_addr + (r->cur_serial * NTB_DATA_MSG_TL);
	//如果msg_len超过128，先copy128B，后面由send进程发送data包
	if (msg_len > NTB_DATA_MSG_TL)
	{
		rte_memcpy(ptr, msg, NTB_DATA_MSG_TL);
	}
	else
	{
		rte_memcpy(ptr, msg, msg_len);
	}
	r->cur_serial = next_serial;
	return 0;
}

static int ntb_msg_dequeue(struct ntb_sublink *sublink, uint64_t *ret_msg_len, char *data)
{
	// struct ntb_ring *r = sublink->local_ring;
	// uint8_t *ptr = r->start_addr + (r->cur_serial * NTB_DATA_MSG_TL);
	// struct ntb_data_msg *msg = (struct ntb_data_msg *)ptr;
	// int msg_len = msg->header.msg_len;
	// //cur_serial no data
	// if (msg_len == 0)
	// {
	// 	return -1;
	// }
	// int msg_type = ntb_mem_header_parser(sublink, msg);
	// //ctrl msg,no need copy to rev_buff
	// if (msg_type != DATA_TYPE)
	// {
	// 	msg->header.msg_len = 0;
	// 	r->cur_serial = (r->cur_serial + 1) % (r->capacity);
	// 	return -1;
	// }
	// *ret_msg_len = msg->header.msg_len - NTB_HEADER_LEN;
	// data = msg->msg;
	// //after dequeue,set the msg_len = 0
	// msg->header.msg_len = 0;
	// r->cur_serial = (r->cur_serial + 1) % (r->capacity);
	// return 0;
}

int ntb_send_data(struct ntb_sublink *sublink, ntp_shm_context_t ring)
{
	uint16_t src_port;
	uint16_t dst_port;
	char *ring_name = ring->shm_addr;
	parser_ring_name(ring_name, &src_port, &dst_port);
	ntp_msg *send_msg = malloc(sizeof(*send_msg));
	uint16_t data_len;
	struct ntb_data_msg *msg = malloc(sizeof(*msg));

	while ((ring->ntsring_handle->shmring->read_index + 1 & 0x03ff) != ring->ntsring_handle->opposite_read_index)
	{
		if (ntp_shm_recv(ring, send_msg) < 0)
		{
			break;
		}
		data_len = send_msg->msg_len;
		if (data_len <= NTB_DATA_MSG_TL - NTB_HEADER_LEN)
		{
			rte_memcpy(msg->msg, (uint8_t *)send_msg->msg, data_len);
			ntb_msg_add_header(msg, src_port, dst_port, data_len, SINGLE_PKG);
			ntb_msg_enqueue(sublink, msg);
		}
		else
		{
			uint16_t sent = 0;
			rte_memcpy(msg->msg, (uint8_t *)send_msg->msg, NTB_DATA_MSG_TL - NTB_HEADER_LEN);
			ntb_msg_add_header(msg, src_port, dst_port, data_len, MULTI_PKG);
			ntb_msg_enqueue(sublink, msg);
			sent += (NTB_DATA_MSG_TL - NTB_HEADER_LEN);
			//下面为纯data包，NTB_DATA_MSG_TL不需要减去header长度
			while (data_len - sent > NTB_DATA_MSG_TL)
			{
				ntb_data_enqueue(sublink, (send_msg->msg + sent), NTB_DATA_MSG_TL);
				sent += NTB_DATA_MSG_TL;
			}
			ntb_data_enqueue(sublink, (send_msg->msg + sent), data_len - sent);
			//发送ENF包表示多个包传输完毕
			ntb_msg_add_header(msg, src_port, dst_port, 0, ENF_MULTI);
			ntb_msg_enqueue(sublink, msg);
		}
	}
	char *conn_name = create_conn_name(msg->src_port, msg->dst_port);
	ntb_conn *conn = Get(ntlink->map, conn_name);
	if (conn->state == CLOSE_SERVER)
	{
		//连接处于关闭状态则发送FIN
		ntb_msg_add_header(msg, src_port, dst_port, 0, FIN_PKG);
		ntb_msg_enqueue(sublink, msg);
	}
	else
	{
		//否则发送探测包
		ntb_msg_add_header(msg, src_port, dst_port, 0, DETECT_PKG);
		ntb_msg_enqueue(sublink, msg);
	}
	return 0;
}

int ntb_receive(struct ntb_sublink *sublink, struct ntb_link *ntlink)
{
	struct ntb_ring *r = sublink->local_ring;
	while (1)
	{
		__asm__("mfence");
		struct ntb_data_msg *msg = (struct ntb_data_msg *)(r->start_addr + (r->cur_serial * NTB_DATA_MSG_TL));
		uint16_t msg_len = msg->header.msg_len;
		msg_len &= 0x0fff;
		//if no msg,continue
		if (msg_len == 0)
		{
			continue;
		}
		// rte_memcpy((uint8_t *)receive_buff + received, msg->msg, msg_len - NTB_HEADER_LEN);
		// r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		int msg_type = ntb_mem_header_parser(sublink, msg);
		//解析接收的包时将src和dst port交换
		uint16_t src_port = msg->header.dst_port;
		uint16_t dst_port = msg->header.src_port;
		char *conn_name = create_conn_name(src_port, dst_port);
		ntb_conn *conn = Get(ntlink->map, conn_name);
		if (conn == NULL || conn->state == CLOSE_CONN)
		{
			//连接已关闭，查找不到连接信息
			uint64_t next_serial = (r->cur_serial + (msg_len + NTB_DATA_MSG_TL - 1) / NTB_DATA_MSG_TL) % (r->capacity);
			for (int i = 0; i <= next_serial - r->cur_serial; i++)
			{
				msg->header.msg_len = 0;
				msg++;
			}
			r->cur_serial = (next_serial + 1) % (r->capacity);
			continue;
		}
		ntp_msg *recv_msg = malloc(sizeof(*recv_msg));
		if (msg_type == SINGLE_PKG)
		{
			recv_msg->msg_type = 0;
			recv_msg->msg_len = msg_len - NTB_HEADER_LEN;
			rte_memcpy(recv_msg->msg, msg->msg, msg_len - NTB_HEADER_LEN);
			ntp_shm_send(conn->recv_ring, recv_msg);
			//将msg_len置为0，cur_serial++
			msg->header.msg_len = 0;
			r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		}
		else if (msg_type == MULTI_PKG)
		{
			recv_msg->msg_type = 0;
			recv_msg->msg_len = msg_len - NTB_HEADER_LEN;
			uint64_t next_serial = (r->cur_serial + (recv_msg->msg_len + NTB_DATA_MSG_TL - 1) / NTB_DATA_MSG_TL) % (r->capacity);
			struct ntb_data_msg *fin_msg = (struct ntb_data_msg *)(r->start_addr + (next_serial * NTB_DATA_MSG_TL));
			while (fin_msg->header.msg_len != 0)
			{
				rte_memcpy(recv_msg->msg, msg->msg, msg_len - NTB_HEADER_LEN);
			}
			ntp_shm_send(conn->recv_ring, recv_msg);
			for (int i = 0; i <= next_serial - r->cur_serial; i++)
			{
				msg->header.msg_len = 0;
				msg++;
			}
			r->cur_serial = (next_serial + 1) % (r->capacity);
		}
		else if (msg_type == DETECT_PKG)
		{
			detect_pkg_handler(ntlink, msg);
			r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		}
		else if (msg_type == FIN_PKG)
		{
			conn->state = CLOSE_CLIENT;
			//收到FIN包将state置为CLOSE—CLIENT。遍历ring_list时判断conn->state来close移出node并free
			r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		}
		else
		{
			ERR("msg_type error");
			r->cur_serial = (r->cur_serial + 1) % (r->capacity);
		}
	}
	return 0;
}

static struct ntb_ring *
ntb_ring_create(uint8_t *ptr, uint64_t ring_size, uint64_t msg_len)
{
	struct ntb_ring *r = malloc(sizeof(*r));
	r->cur_serial = 0;
	r->start_addr = ptr;
	r->end_addr = r->start_addr + ring_size;
	r->capacity = ring_size / msg_len;
	NTB_LOG(DEBUG, "ring_capacity == %ld.", r->capacity);
	return r;
}

static int ntb_mem_formatting(struct ntb_link *ntb_link, uint8_t *local_ptr, uint8_t *remote_ptr)
{
	ntb_link->ctrllink->local_cum_ptr = (uint64_t *)local_ptr;
	ntb_link->ctrllink->remote_cum_ptr = (uint64_t *)remote_ptr;
	ntb_link->ctrllink->local_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), CTRL_RING_SIZE, NTB_CTRL_MSG_TL);
	ntb_link->ctrllink->remote_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), CTRL_RING_SIZE, NTB_CTRL_MSG_TL);
	local_ptr = (uint8_t *)local_ptr + RING_SIZE + 8;
	remote_ptr = (uint8_t *)remote_ptr + RING_SIZE + 8;
	struct ntp_config *cf = get_ntp_config();
	for (int i = 0; i < cf->nt_proxy.sublink_number; i++)
	{
		ntb_link->sublink[i].local_cum_ptr = (uint64_t *)local_ptr;
		ntb_link->sublink[i].remote_cum_ptr = (uint64_t *)remote_ptr;
		*ntb_link->sublink[i].local_cum_ptr = 0;
		ntb_link->sublink[i].local_ring = ntb_ring_create(((uint8_t *)local_ptr + 8), RING_SIZE, NTB_DATA_MSG_TL);
		ntb_link->sublink[i].remote_ring = ntb_ring_create(((uint8_t *)remote_ptr + 8), RING_SIZE, NTB_DATA_MSG_TL);
		local_ptr = (uint8_t *)local_ptr + RING_SIZE + 8;
		remote_ptr = (uint8_t *)remote_ptr + RING_SIZE + 8;
	}
	return 0;
}

struct ntb_link *
ntb_start(uint16_t dev_id)
{
	struct rte_rawdev *dev;
	NTB_LOG(DEBUG, "Start dev_id=%d.", dev_id);

	dev = &rte_rawdevs[dev_id];

	struct ntp_config *cf = get_ntp_config();

	struct ntb_link *ntb_link = malloc(sizeof(*ntb_link));
	ntb_link->dev = dev;
	ntb_link->hw = dev->dev_private;
	ntb_link->sublink = malloc(sizeof(struct ntb_sublink) * cf->nt_proxy.sublink_number);
	int diag;

	if (dev->started != 0)
	{
		NTB_LOG(ERR, "Device with dev_id=%d already started",
				dev_id);
		return 0;
	}

	diag = (*dev->dev_ops->dev_start)(dev);

	if (diag == 0)
		dev->started = 1;
	//mx_idx = 0 or 1
	uint8_t *local_ptr = (uint8_t *)ntb_link->hw->mz[0]->addr;
	uint8_t *remote_ptr = (uint8_t *)ntb_link->hw->pci_dev->mem_resource[2].addr;
	ntb_mem_formatting(ntb_link, local_ptr, remote_ptr);
	struct ntp_ring_list_node *ring_node = malloc(sizeof(*ring_node));
	ring_node->conn = NULL;
	ring_node->next_node = ring_node;
	ntb_link->ring_head = ring_node;
	ntb_link->ring_tail = ring_node;
	ntb_link->map = createHashMap(NULL, NULL);
	nts_shm_context_t ntm_ntp = ntm_shm();
	char *ntm_ntp_name = "ntm-ntp";
	ntm_shm_accept(ntm_ntp, ntm_ntp_name, sizeof(ntm_ntp_name));
	ntb_link->ntm_ntp = ntm_ntp;

	nts_shm_context_t ntp_ntm = ntm_shm();
	char *ntp_ntm_name = "ntp-ntm";
	ntm_shm_accept(ntp_ntm, ntp_ntm_name, sizeof(ntp_ntm_name));
	ntb_link->ntp_ntm = ntp_ntm;

	return ntb_link;
}
