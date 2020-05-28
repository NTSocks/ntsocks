/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Intel Corporation
 */

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <rte_io.h>
#include <rte_eal.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_rawdev.h>
#include <rte_rawdev_pmd.h>
#include <rte_memcpy.h>
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

#include "ntp_func.h"
#include "nt_log.h"
#include "config.h"

// uint64_t global_rece_data = 0;
// uint64_t global_send_data = 0;
// uint64_t global_rece_msg = 0;
// uint64_t global_send_msg = 0;

DEBUG_SET_LEVEL(DEBUG_LEVEL_DISABLE);

static char *int_to_char(uint16_t x)
{
    char *str = (char *)malloc(6);
    sprintf(str, "%d", x);
    return str;
}

//result[0] is src_port-dst_post-s, result[1] is src_port-des_port-r
// static int parse_sockaddr_port(struct sockaddr_in *saddr)
// {
//     assert(saddr);
//     int port;
//     port = ntohs(saddr->sin_port);
//     return port;
// }
// static char **create_nts_ring_name(uint16_t src_port, uint16_t dst_port)
// {
//     char **result = 0;
//     result = malloc(sizeof(char *) * 2);
//     result[0] = malloc(sizeof(char) * 14);
//     result[1] = malloc(sizeof(char) * 14);
//     char *bar = "-";
//     char *send_buff = "s";
//     char *recv_buff = "r";
//     char *src_port_str = int_to_char(src_port);
//     char *dst_port_str = int_to_char(dst_port);

//     strcpy(result[0], src_port_str);
//     strcat(result[0], bar);
//     strcat(result[0], dst_port_str);
//     strcat(result[0], bar);
//     strcat(result[0], send_buff);

//     strcpy(result[1], src_port_str);
//     strcat(result[1], bar);
//     strcat(result[1], dst_port_str);
//     strcat(result[1], bar);
//     strcat(result[1], recv_buff);
//     DEBUG("name 1 = %s,name 2 = %s", *result, *(result + 1));
//     free(src_port_str);
//     free(dst_port_str);

//     return result;
// }

static int add_conn_to_ntb_send_list(struct ntb_link_custom *ntb_link, ntb_conn *conn)
{
    ntp_send_list_node *list_node = malloc(sizeof(*list_node));
    list_node->conn = conn;
    list_node->next_node = ntb_link->send_list.ring_head;
    ntb_link->send_list.ring_tail->next_node = list_node;
    ntb_link->send_list.ring_tail = list_node;
    return 0;
}

static uint32_t create_conn_id(uint16_t src_port, uint16_t dst_port)
{
    uint32_t conn_name;
    conn_name = (src_port << 16);
    conn_name += dst_port;
    return conn_name;
}

// send `SYN_ACK` back to ntb monitor
static int ntp_create_conn_ack(struct ntb_link_custom *ntb_link, ntm_ntp_msg *msg)
{
    char *test_name = "test";
    ntp_ntm_msg reply_msg;
    reply_msg.src_ip = msg->src_ip;
    reply_msg.dst_ip = msg->dst_ip;
    reply_msg.src_port = msg->src_port;
    reply_msg.dst_port = msg->dst_port;
    reply_msg.msg_type = CREATE_CONN_ACK;
    reply_msg.msg_len = 48;

    memcpy(reply_msg.msg, test_name, 4);

    ntp_ntm_shm_send(ntb_link->ntp_ntm, &reply_msg);
    DEBUG("msg_type=%d", reply_msg.msg_type);
    return 0;
}

int ntp_create_conn_handler(struct ntb_link_custom *ntb_link, ntm_ntp_msg *msg)
{
    ntb_conn *conn = malloc(sizeof(*conn));
    conn->state = READY_CONN; // update the ntb connection to `READY_DOWN`

    ntp_shm_context_t send_ring = ntp_shm(); // create shm ring
    ntp_shm_context_t recv_ring = ntp_shm();

    // char **nts_ring_name = create_nts_ring_name(msg->src_port, msg->dst_port);
    int src_port, dst_port;
    char recv_shmaddr[SHM_NAME_LEN], send_shmaddr[SHM_NAME_LEN];
    src_port = ntohs(msg->src_port);
    dst_port = ntohs(msg->dst_port);
    sprintf(recv_shmaddr, "%d-%d-r", src_port, dst_port);
    sprintf(send_shmaddr, "%d-%d-s", src_port, dst_port);
    DEBUG("ntp recv_shmaddr=%s, send_shmaddr=%s", recv_shmaddr, send_shmaddr);

    conn->conn_id = create_conn_id(src_port, dst_port); // generate ntb connection name
    // create the send/recv buffer for ntb connection
    // if (ntp_shm_accept(send_ring, nts_ring_name[0], sizeof(nts_ring_name[0])) != 0)
    if (ntp_shm_accept(send_ring, send_shmaddr, strlen(send_shmaddr)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
    // if (ntp_shm_accept(recv_ring, nts_ring_name[1], sizeof(nts_ring_name[1])) != 0)
    if (ntp_shm_accept(recv_ring, recv_shmaddr, strlen(recv_shmaddr)) != 0)
    {
        DEBUG("create ntm_ntp_ring failed\n");
    }
    // free(nts_ring_name[0]);
    // free(nts_ring_name[1]);
    // free(nts_ring_name);

    conn->nts_send_ring = send_ring;
    conn->nts_recv_ring = recv_ring;

    // hold/save the new ntb connection into hash map
    Put(ntb_link->port2conn, &conn->conn_id, conn);
    HashMapIterator iter = createHashMapIterator(ntb_link->port2conn);
    while (hasNextHashMapIterator(iter))
    {
        iter = nextHashMapIterator(iter);
        struct ntb_conn *tmp_conn = (ntb_conn *)iter->entry->value;
        DEBUG("{ key = %d }", *(int *)iter->entry->key);
    }
    freeHashMapIterator(&iter);

    add_conn_to_ntb_send_list(ntb_link, conn);

    ntp_create_conn_ack(ntb_link, msg);

    DEBUG("create conn success,conn name is %d", conn->conn_id);

    return 0;
}

static int detect_pkg_handler(struct ntb_link_custom *ntb_link, uint16_t src_port, uint16_t dst_port, ntb_conn *conn)
{
    struct ntb_ctrl_msg reply_msg;
    uint64_t read_index = ntp_get_read_index(conn->nts_recv_ring->ntsring_handle);
    //不需要调换，因为conn已经解析好
    reply_msg.header.src_port = src_port;
    reply_msg.header.dst_port = dst_port;
    reply_msg.header.msg_len = 6;

    rte_memcpy(reply_msg.msg, &read_index, 8);
    ntb_ctrl_msg_enqueue(ntb_link, &reply_msg);
    INFO("send my read_index = %ld", read_index);
    return 0;
}

int ntp_send_buff_data(struct ntb_data_link *data_link, ntp_shm_context_t ring, ntb_conn *conn)
{
    uint16_t src_port = (uint16_t)(conn->conn_id >> 16);
    uint16_t dst_port = (uint16_t)conn->conn_id;

    //recv send_msg from nts_send_ring, slice and package as ntb_data_msg
    uint64_t detect_interval_time = DETECT_INTERVAL * 25000;
    ntp_msg *send_msg;
    uint16_t data_len;
    struct ntb_data_msg packaged_msg; // indicate the element of data ntb_data_link ring == part/segment of ntb buffer

    // the read index of peer node is compared with current read_index of send buffer in one ntb connection,
    // judege whether local read_index cross the border (the peer read_index)
    uint64_t read_index = ntp_get_read_index(ring->ntsring_handle);
    uint64_t peer_read_index = ntp_get_peer_read_index(ring->ntsring_handle);
    uint64_t send_window = peer_read_index - read_index > 0 ? peer_read_index - read_index - 1 : peer_read_index - read_index + NTS_MAX_BUFS - 1;
    int i;
    for (i = 0; i < NTP_CONFIG.branch_trans_number; i++)
    {
        send_msg = ntp_shm_recv(ring);
        if (send_msg == NULL)
        {
            // DEBUG("send buff is empty");
            break;
        }
        if (send_msg->msg_type == NTP_FIN) // when NTP_FIN packet/msg, update ntb-conn state as `ACTIVE_CLOSE`
        {
            //如果检测到FIN包，发送FIN_PKG to peer，close队列，并将conn->state置为ACTIVE_CLOSE
            //正常APP的FIN包后不会再有数据包，直接return
            ntb_data_msg_add_header(&packaged_msg, src_port, dst_port, 0, FIN_PKG);
            ntb_data_msg_enqueue(data_link, &packaged_msg);
            conn->state = ACTIVE_CLOSE;
            DEBUG("conn->state change to active_close");
            return 0;
        }
        data_len = send_msg->msg_len;
        if (data_len <= DATA_MSG_LEN)
        {
            rte_memcpy(packaged_msg.msg, (uint8_t *)send_msg->msg, data_len);
            ntb_data_msg_add_header(&packaged_msg, src_port, dst_port, data_len, SINGLE_PKG);
            ntb_data_msg_enqueue(data_link, &packaged_msg);
        }
        else
        {
            rte_memcpy(packaged_msg.msg, (uint8_t *)send_msg->msg, DATA_MSG_LEN);
            ntb_data_msg_add_header(&packaged_msg, src_port, dst_port, data_len, MULTI_PKG);
            // DEBUG("multi pkg header,src_port = %d ,dst_port=%d,msg_len = %d", src_port, dst_port, data_len);
            ntb_data_msg_enqueue(data_link, &packaged_msg);
            uint16_t sent = DATA_MSG_LEN;
            //下面为纯data包，NTB_DATA_MSG_TL不需要减去header长度
            while (data_len - sent > NTB_DATA_MSG_TL)
            {
                ntb_pure_data_msg_enqueue(data_link, (uint8_t *)(send_msg->msg + sent), NTB_DATA_MSG_TL);
                // DEBUG("add pure_data,sent_len = %d", sent);
                sent += NTB_DATA_MSG_TL;
            }
            ntb_pure_data_msg_enqueue(data_link, (uint8_t *)(send_msg->msg + sent), data_len - sent);
            // DEBUG("add pure_data,sent_len = %d", sent);
            //发送ENF包表示多个包传输完毕
            ntb_data_msg_add_header(&packaged_msg, src_port, dst_port, 0, ENF_MULTI); // the ENF_MULTI packet: empty data packet
            // DEBUG("ENF_MULTI pkg enqueue");
            ntb_data_msg_enqueue(data_link, &packaged_msg);
        }
        // global_send_data += send_msg->msg_len;
        // global_send_msg += 1;
        // DEBUG("send_msg counter = %ld,send_msg_len counter = %ld", global_send_msg, global_send_data);
        shm_mempool_node *tmp_node;
        tmp_node = shm_mp_node_by_shmaddr(ring->mp_handler, (char *)send_msg);
        if (tmp_node)
        {
            shm_mp_free(ring->mp_handler, tmp_node);
        }
    }
    i = 0;
    //如果当前send_window < 512 且 当前时间 - 上一次发送探测包时间 > 4us，则发送探测包
    uint64_t current_time = rte_rdtsc();
    if (send_window - i < 512 && current_time - conn->detect_time > detect_interval_time)
    {
        ntb_data_msg_add_header(&packaged_msg, src_port, dst_port, 0, DETECT_PKG);
        ntb_data_msg_enqueue(data_link, &packaged_msg);
        conn->detect_time = current_time;
    }
    return 0;
}

int ntp_receive_data_to_buff(struct ntb_data_link *data_link, struct ntb_link_custom *ntb_link)
{
    struct ntb_ring_buffer *r = data_link->local_ring;
    volatile struct ntb_data_msg *msg, *fin_msg;
    uint16_t msg_len, first_len;
    uint8_t msg_type;
    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    uint32_t conn_id = 0;
    ntb_conn *conn;
    int i, count;
    uint64_t next_index, fin_index, temp_index;

    while (1)
    {
        msg = (struct ntb_data_msg *)(r->start_addr + (r->cur_index << 7));
        msg_len = msg->header.msg_len;
        //if no msg,continue
        if (msg_len == 0)
        {
            continue;
        }
        msg_len &= 0x0fff; // compute real msg_len == end 12 bits
        msg_type = parser_data_len_get_type(data_link, msg->header.msg_len);
        //如果当前包端口号与上次比有所改变，需要重新Get conn
        if (src_port != msg->header.dst_port || dst_port != msg->header.src_port)
        {
            HashMapIterator iter = createHashMapIterator(ntb_link->port2conn);
            while (hasNextHashMapIterator(iter))
            {
                iter = nextHashMapIterator(iter);
                struct ntb_conn *tmp_conn = (ntb_conn *)iter->entry->value;
                // DEBUG("{ key = %d }", *(int *)iter->entry->key);
            }
            freeHashMapIterator(&iter);

            DEBUG("Get conn start");
            //解析接收的包时将src和dst port交换
            src_port = msg->header.dst_port;
            dst_port = msg->header.src_port;
            conn_id = create_conn_id(src_port, dst_port);
            conn = (ntb_conn *)Get(ntb_link->port2conn, &conn_id);
            // DEBUG("search conn_id = %d", conn_id);
            // DEBUG("Get conn end");
        }
        //当前消息的端口号与上一条消息完全相等时，为同一条conn的消息，将不会重复进行get
        if (conn == NULL || conn->state == ACTIVE_CLOSE || conn->state == PASSIVE_CLOSE)
        { // if the ntb conn is close state, then drop current nt_packet
            if (conn == NULL)
            {
                // DEBUG("conn = null");
            }
            else
            {
                // DEBUG("conn state = %d", conn->state);
            }
            count = (msg_len + NTB_DATA_MSG_TL - 1) >> 7;
            temp_index = r->cur_index;
            //会将ENF_MULTI包所在位置也清零
            count = count > 1 ? count + 1 : count;
            for (i = 0; i < count; i++)
            {
                temp_index = temp_index + 1 < r->capacity ? temp_index + 1 : 0;
                msg = (struct ntb_data_msg *)(r->start_addr + (temp_index << 7));
                msg->header.msg_len = 0;
            }
            r->cur_index = temp_index + 1 < r->capacity ? temp_index + 1 : 0;
            continue;
        }
        //switch to the libfdu-utils
        shm_mempool_node *mp_node;
        mp_node = shm_mp_malloc(conn->nts_recv_ring->mp_handler, sizeof(ntp_msg));
        if (mp_node == NULL)
        {
            perror("shm_mp_malloc failed. \n");
            return -1;
        }

        ntp_msg *recv_msg = (ntp_msg *)shm_offset_mem(conn->nts_recv_ring->mp_handler, mp_node->node_idx);
        if (recv_msg == NULL)
        {
            perror("shm_offset_mem failed \n");
            return -1;
        }
        // ntp_msg recv_msg;

        if (msg_type == SINGLE_PKG)
        {
            // DEBUG("receive SINGLE_PKG start");
            recv_msg->msg_type = NTP_DATA;
            recv_msg->msg_len = msg_len - NTB_HEADER_LEN;
            rte_memcpy(recv_msg->msg, msg->msg, recv_msg->msg_len);
            ntp_shm_send(conn->nts_recv_ring, recv_msg);
            //将msg_len置为0，cur_index++
            msg->header.msg_len = 0;
            // if(r->cur_index +1 >= max_cap)5
            r->cur_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
            // global_rece_data += recv_msg->msg_len;
            // global_rece_msg += 1;
            // DEBUG("recv_msg counter = %ld,rece_msg_len counter = %ld", global_rece_msg, global_rece_data);
            // DEBUG("recv_msg->msg = %s", recv_msg->msg);
            // DEBUG("receive SINGLE_PKG end");
        }
        else if (msg_type == MULTI_PKG)
        {
            // DEBUG("receive MULTI_PKG start");
            recv_msg->msg_type = NTP_DATA;
            recv_msg->msg_len = msg_len - NTB_HEADER_LEN;
            temp_index = r->cur_index;
            next_index = temp_index + ((msg_len + NTB_DATA_MSG_TL - 1) >> 7); //fix recv_msg->msg_len to msg_len
            fin_index = next_index < r->capacity ? next_index : next_index - r->capacity;
            //fin_msg 为ENF_MULTI包应该抵达的位置
            fin_msg = (struct ntb_data_msg *)(r->start_addr + (fin_index << 7));
            while (fin_msg->header.msg_len == 0)
            {
                // INFO("waiting for enf_mulit msg,waiting_index = %ld", fin_index);
            }
            if (next_index <= r->capacity)
            {
                rte_memcpy(recv_msg->msg, msg->msg, recv_msg->msg_len);
            }
            else
            {
                first_len = ((r->capacity - temp_index - 1) << 7) + DATA_MSG_LEN;
                rte_memcpy(recv_msg->msg, msg->msg, first_len);
                rte_memcpy((recv_msg->msg + first_len), r->start_addr, recv_msg->msg_len - first_len);
            }
            //将所有接收到的消息清空
            for (i = 0; i <= next_index - r->cur_index; i++)
            {
                msg = (struct ntb_data_msg *)(r->start_addr + (temp_index << 7));
                msg->header.msg_len = 0;
                temp_index = temp_index + 1 < r->capacity ? temp_index + 1 : 0;
            }
            // global_rece_data += recv_msg->msg_len;
            // global_rece_msg += 1;
            // DEBUG("recv_msg counter = %ld,rece_msg_len counter = %ld", global_rece_msg, global_rece_data);
            ntp_shm_send(conn->nts_recv_ring, recv_msg);
            r->cur_index = next_index + 1 < r->capacity ? next_index + 1 : next_index - r->capacity + 1;
            // DEBUG("recv_msg->msg = %s", recv_msg->msg);
            // DEBUG("receive MULTI_PKG end");
        }
        else if (msg_type == DETECT_PKG)
        {
            INFO("receive DETECT_PKG");
            detect_pkg_handler(ntb_link, msg->header.src_port, msg->header.dst_port, conn); // send the response msg of DETECT_PKG to ntb control msg link (peer node)
            msg->header.msg_len = 0;
            r->cur_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
        }
        else if (msg_type == FIN_PKG)
        {
            INFO("receive FIN_PKG");
            recv_msg->msg_type = NTP_FIN;
            recv_msg->msg_len = msg_len - NTB_HEADER_LEN;
            ntp_shm_send(conn->nts_recv_ring, recv_msg);
            conn->state = PASSIVE_CLOSE;
            // DEBUG("conn->state change to passive_close");
            msg->header.msg_len = 0;
            //收到FIN包将state置为CLOSE—CLIENT。遍历ring_list时判断conn->state来close移出node并free
            r->cur_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
        }
        else
        {
            ERR("msg_type error");
            msg->header.msg_len = 0;
            r->cur_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
        }
    }
    return 0;
}

int ntp_ctrl_msg_receive(struct ntb_link_custom *ntb_link)
{
    struct ntb_ring_buffer *r = ntb_link->ctrl_link->local_ring;
    volatile struct ntb_ctrl_msg *msg;
    uint16_t msg_len;
    uint16_t src_port, dst_port;
    uint32_t conn_id;
    while (1)
    {
        msg = (struct ntb_ctrl_msg *)(r->start_addr + (r->cur_index << 4));
        msg_len = msg->header.msg_len;
        // msg_len &= 0x0fff;
        //if no msg,continue
        if (msg_len == 0)
        {
            continue;
        }
        DEBUG("receive a ntb_ctrl_msg");
        parser_ctrl_msg_header(ntb_link, msg->header.msg_len);
        //解析接收的包时将src和dst port交换
        src_port = msg->header.dst_port;
        dst_port = msg->header.src_port;
        conn_id = create_conn_id(src_port, dst_port);
        ntb_conn *conn = (ntb_conn *)Get(ntb_link->port2conn, &conn_id);
        ntp_set_peer_read_index(conn->nts_send_ring->ntsring_handle, *(uint64_t *)msg->msg);
        r->cur_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
        msg->header.msg_len = 0;
    }
    return 0;
}
