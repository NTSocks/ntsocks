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
#include <string.h>

#include "ntp_func.h"
#include "config.h"
#include "utils.h"
#include "nt_log.h"
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

// uint64_t global_rece_data = 0;
// uint64_t global_send_data = 0;
// uint64_t global_rece_msg = 0;
// uint64_t global_send_msg = 0;

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

static int add_conn_to_ntb_send_list(struct ntb_link_custom *ntb_link, ntb_partition_t partition, ntb_conn *conn)
{
    DEBUG("add_conn_to_ntb_send_list with partition_id=%d", conn->partition_id);
    assert(partition);

    ntp_send_list_node *list_node = (ntp_send_list_node *) malloc(sizeof(*list_node));
    list_node->conn = conn;
    list_node->next_node = partition->send_list.ring_head;
    partition->send_list.ring_tail->next_node = list_node;
    partition->send_list.ring_tail = list_node;
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
static int ntp_create_conn_ack(struct ntb_link_custom *ntb_link, ntb_partition_t partition, ntm_ntp_msg *msg)
{
    char *test_name = "test";
    ntp_ntm_msg reply_msg;
    reply_msg.src_ip = msg->src_ip;
    reply_msg.dst_ip = msg->dst_ip;
    reply_msg.src_port = msg->src_port;
    reply_msg.dst_port = msg->dst_port;
    reply_msg.msg_type = CREATE_CONN_ACK;
    if (msg->partition_id < 0) {    // accept side
        reply_msg.partition_id = partition->id;
    } 
    reply_msg.msg_len = strlen(test_name);

    memcpy(reply_msg.msg, test_name, reply_msg.msg_len);

    ntp_ntm_shm_send(ntb_link->ntp_ntm, &reply_msg);
    DEBUG("msg_type=%d", reply_msg.msg_type);
    return 0;
}

int ntp_create_conn_handler(struct ntb_link_custom *ntb_link, ntm_ntp_msg *msg)
{
    ntb_conn *conn = malloc(sizeof(*conn));
    conn->state = READY_CONN; // update the ntb connection to `READY_DOWN`

    ntp_shm_context_t send_ring = ntp_shm(NTP_CONFIG.data_packet_size); // create shm ring
    ntp_shm_context_t recv_ring = ntp_shm(NTP_CONFIG.data_packet_size);

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
    // HashMapIterator iter = createHashMapIterator(ntb_link->port2conn);
    // while (hasNextHashMapIterator(iter))
    // {
    //     iter = nextHashMapIterator(iter);
    //     struct ntb_conn *tmp_conn = (ntb_conn *)iter->entry->value;
    //     DEBUG("{ key = %d }", *(int *)iter->entry->key);
    // }
    // freeHashMapIterator(&iter);

    /**
     *  Init the ntb_partition for ntb_conn
     * 1. If accept side, allocate partition
     * 2. Else if connect side, receive the partition_id from accept side and set it in local ntb_conn
     */
    if (msg->partition_id >= 0) {   // connect side
        INFO("connect side with partition_id=%d", msg->partition_id);
        conn->partition_id = msg->partition_id;
        conn->partition = &ntb_link->partitions[conn->partition_id];
        
    } else {    // accept side
        uint16_t partition_idx = ntp_allocate_partition(ntb_link);
        INFO("accept side with partition_id=%d", partition_idx);
        conn->partition_id = partition_idx;
        conn->partition = &ntb_link->partitions[partition_idx];
    }
    conn->partition->num_conns++;
    
    add_conn_to_ntb_send_list(ntb_link, conn->partition, conn);

    ntp_create_conn_ack(ntb_link, conn->partition, msg);

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

/**
 * @brief  ntb_partition send the messages of specified ntb_conn (ntp_shm_context_t)
 * @note   
 * @param  *data_link: 
 * @param  partition: 
 * @param  ring: 
 * @param  *conn: 
 * @retval return the number of actually sent messages
 */
int ntp_send_buff_data(struct ntb_data_link *data_link, ntb_partition_t partition, ntp_shm_context_t ring, ntb_conn *conn)
{
    uint16_t src_port = (uint16_t)(conn->conn_id >> 16);
    uint16_t dst_port = (uint16_t)conn->conn_id;

    //recv send_msg from nts_send_ring, slice and package as ntpacket
    uint64_t detect_interval_time = DETECT_INTERVAL * 25000;
    ntp_msg send_msg;
    uint16_t data_len;
    struct ntpacket data_packet; // indicate the element of data ntb_data_link ring == part/segment of ntb buffer
    int rc;

    // the read index of peer node is compared with current read_index of send buffer in one ntb connection,
    // judge whether local read_index cross the border (the peer read_index)
    uint64_t read_index = ntp_get_read_index(ring->ntsring_handle);
    uint64_t peer_read_index = ntp_get_peer_read_index(ring->ntsring_handle);
    uint64_t send_window = peer_read_index - read_index > 0 ? peer_read_index - read_index - 1 : peer_read_index - read_index + NTS_MAX_BUFS - 1;
    
    int i;
    // bulk packet forwarding from libnts send shmring
    size_t recv_cnt = 0;
    // recv_cnt = ntp_shm_recv_bulk(ring, partition->cache_msg_bulks, NTP_CONFIG.bulk_size);
    // DEBUG("ntp_shm_recv_bulk recv_cnt = %d", (int)recv_cnt);
    // if (recv_cnt == FAILED)  return FAILED;
    
    // NTP_CONFIG.bulk_size is equal to batch size
    for (i = 0; i < NTP_CONFIG.bulk_size; i++)
    {
        // send_msg = partition->cache_msg_bulks[i];
        rc = ntp_shm_recv(ring, &send_msg);
        if (rc == -1)
        {
            // DEBUG("send buff is empty");
            break;
        }
        recv_cnt++;
        // if (strlen(send_msg.payload))
        //     printf("send_msg:%s \n", send_msg.payload);

        if (UNLIKELY(send_msg.header->msg_type == NTP_FIN)) // when NTP_FIN packet/msg, update ntb-conn state as `ACTIVE_CLOSE`

        {
            //如果检测到FIN包，发送FIN_PKG to peer，close队列，并将conn->state置为ACTIVE_CLOSE
            //正常APP的FIN包后不会再有数据包，直接return
            pack_ntpacket_header(send_msg.header, src_port, dst_port, 0, FIN_PKG);
            ntpacket_enqueue(data_link, send_msg.header, &send_msg);
            // ntb_data_msg_add_header(&data_packet, src_port, dst_port, 0, FIN_PKG);
            // ntb_data_msg_enqueue(data_link, &data_packet);
            conn->state = ACTIVE_CLOSE;
            DEBUG("conn->state change to active_close");
            return 0;
        }

        data_len = send_msg.header->msg_len;
        if (LIKELY(data_len <= NTP_CONFIG.datapacket_payload_size))
        {
            pack_ntpacket_header(send_msg.header, src_port, dst_port, data_len, SINGLE_PKG);
            ntpacket_enqueue(data_link, send_msg.header, &send_msg);
            // ntb_data_msg_enqueue2(data_link, &send_msg, 
            //             src_port, dst_port, data_len, SINGLE_PKG);
        }
        else
        {
            ntb_data_msg_enqueue2(data_link, &send_msg, 
                        src_port, dst_port, data_len, MULTI_PKG);

            // DEBUG("multi pkg header,src_port = %d ,dst_port=%d,msg_len = %d", src_port, dst_port, data_len);
            uint16_t sent = NTP_CONFIG.datapacket_payload_size;
            //下面为纯data包，NTP_CONFIG.data_packet_size 不需要减去header长度
            while (data_len - sent > NTP_CONFIG.data_packet_size)
            {
                ntb_pure_data_msg_enqueue(data_link, (uint8_t *)(send_msg.payload + sent), NTP_CONFIG.data_packet_size);
                // DEBUG("add pure_data,sent_len = %d", sent);
                sent += NTP_CONFIG.data_packet_size;
            }

            ntb_pure_data_msg_enqueue(data_link, (uint8_t *)(send_msg.payload + sent), data_len - sent);

            // DEBUG("add pure_data,sent_len = %d", sent);
            //发送ENF包表示多个包传输完毕
            //TODO: can be optimized in the last data packet: with extended header 2 bits, header size(8)
            ntb_data_msg_add_header(&data_packet, src_port, dst_port, 0, ENF_MULTI); // the ENF_MULTI packet: empty data packet
            // DEBUG("ENF_MULTI pkg enqueue");
            ntb_data_msg_enqueue(data_link, &data_packet);
        }

        // global_send_data += send_msg.header->msg_len;
        // global_send_msg += 1;
        // DEBUG("send_msg counter = %ld,send_msg_len counter = %ld", global_send_msg, global_send_data);
        ntp_shm_ntpacket_free(ring, &send_msg);
    }

    i = 0;
    //如果当前send_window < 512 且 当前时间 - 上一次发送探测包时间 > 4us，则发送探测包
    uint64_t current_time = rte_rdtsc();
    if (UNLIKELY(send_window - i < 512 && current_time - conn->detect_time > detect_interval_time))
    {
        ntb_data_msg_add_header(&data_packet, src_port, dst_port, 0, DETECT_PKG);
        ntb_data_msg_enqueue(data_link, &data_packet);
        conn->detect_time = current_time;
    }

    return recv_cnt;
}

int ntp_receive_data_to_buff(struct ntb_data_link *data_link, struct ntb_link_custom *ntb_link, ntb_partition_t partition)
{
    struct ntb_ring_buffer *recv_dataring = data_link->local_ring;  // local NTB-based data ringbuffer
    volatile struct ntpacket msg, fin_msg;
    uint16_t msg_len, first_len;
    uint8_t msg_type;
    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    uint32_t conn_id = 0;
    ntb_conn *conn;
    int i, count;
    uint64_t next_index, fin_index, temp_index;
    int rc;
    void * packet_ptr;

    while (!ntb_link->is_stop)
    {
        packet_ptr = (void *)(recv_dataring->start_addr + 
                    (recv_dataring->cur_index << NTP_CONFIG.ntb_packetbits_size));
        rc = parse_ntpacket(packet_ptr, &msg);
        if (rc == -1)   continue;

        // if no msg,continue
        if (msg.header && msg.header->msg_len == 0)
        {
            continue;
        }

        msg_len = msg.header->msg_len;
        partition->recv_packet_counter++;
        // if (msg.payload)
        //     printf("msg.payload: %s \n", msg.payload);

        msg_len &= 0x0fff; // compute real msg_len == end 12 bits
        msg_type = parser_data_len_get_type(data_link, msg.header->msg_len);
        //如果当前包端口号与上次比有所改变，需要重新Get conn
        if (src_port != msg.header->dst_port || dst_port != msg.header->src_port)
        {
            //解析接收的包时将src和dst port交换
            src_port = msg.header->dst_port;
            dst_port = msg.header->src_port;
            conn_id = create_conn_id(src_port, dst_port);
            conn = (ntb_conn *)Get(ntb_link->port2conn, &conn_id);
            // DEBUG("search conn_id = %d", conn_id);
        }

        //当前消息的端口号与上一条消息完全相等时，为同一条conn的消息，将不会重复进行get
        if (UNLIKELY(conn == NULL || conn->state == ACTIVE_CLOSE || conn->state == PASSIVE_CLOSE))
        { // if the ntb conn is close state, then drop current nt_packet
            
            count = (msg_len + NTP_CONFIG.data_packet_size - 1) >> NTP_CONFIG.ntb_packetbits_size;
            temp_index = recv_dataring->cur_index;
            //会将ENF_MULTI包所在位置也清零
            count = count > 1 ? count + 1 : count;
            for (i = 0; i < count; i++)
            {
                temp_index = temp_index + 1 < recv_dataring->capacity ? temp_index + 1 : 0;
                packet_ptr = (void *)(recv_dataring->start_addr + (temp_index << NTP_CONFIG.ntb_packetbits_size));
                rc = parse_ntpacket(packet_ptr, &msg);
                if (rc == -1 || !msg.header) {
                    continue;
                }
                msg.header->msg_len = 0;
            }
            recv_dataring->cur_index = temp_index + 1 < recv_dataring->capacity ? temp_index + 1 : 0;
            continue;
        }

        // switch to the libfdu-utils
        ntp_msg recv_msg;
        rc = ntp_shm_ntpacket_alloc(conn->nts_recv_ring, 
                        &recv_msg, NTP_CONFIG.data_packet_size);
        if (rc == -1) {
            ERR("ntp_shm_ntpacket_alloc failed");
            return -1;
        }

        // report the statistics about the used element ratio of data ringbuffer,
        // if write_idx > read_idx, then write_idx - read_idx
        // else, r->capacity - (read_idx - write_idx) 
        // if ()

        if (msg_type == SINGLE_PKG)
        {
            DEBUG("receive SINGLE_PKG start");
            recv_msg.header->msg_type = NTP_DATA;
            recv_msg.header->msg_len = msg_len - NTPACKET_HEADER_LEN;
            rte_memcpy(recv_msg.payload, msg.payload, recv_msg.header->msg_len);


            rc = ntp_shm_send(conn->nts_recv_ring, &recv_msg);
            if (UNLIKELY(rc == -1)) {
                ERR("ntp_shm_send failed");
                return -1;
            }

            //将msg_len置为0，cur_index++
            msg.header->msg_len = 0;
            // if(recv_dataring->cur_index +1 >= max_cap)
            recv_dataring->cur_index = recv_dataring->cur_index + 1 < recv_dataring->capacity ? recv_dataring->cur_index + 1 : 0;
            // global_rece_data += recv_msg.header->msg_len;
            // global_rece_msg += 1;
            // DEBUG("recv_msg counter = %ld,rece_msg_len counter = %ld", global_rece_msg, global_rece_data);
            // DEBUG("recv_msg.payload = %s", recv_msg.payload);
            // DEBUG("receive SINGLE_PKG end");
        }
        else if (msg_type == MULTI_PKG)
        {
            // DEBUG("receive MULTI_PKG start");
            recv_msg.header->msg_type = NTP_DATA;
            recv_msg.header->msg_len = msg_len - NTPACKET_HEADER_LEN;
            temp_index = recv_dataring->cur_index;
            next_index = temp_index + ((msg_len + NTP_CONFIG.data_packet_size - 1) >> NTP_CONFIG.ntb_packetbits_size); // fix recv_msg.header->msg_len to msg_len
            fin_index = next_index < recv_dataring->capacity ? next_index : next_index - recv_dataring->capacity;
            //fin_msg 为ENF_MULTI包应该抵达的位置
            packet_ptr = (void *)(recv_dataring->start_addr + (fin_index << NTP_CONFIG.ntb_packetbits_size));
            rc = parse_ntpacket(packet_ptr, &fin_msg);
            if (UNLIKELY(rc == -1))   return -1;

            while (UNLIKELY(fin_msg.header->msg_len == 0))
            {
                // INFO("waiting for enf_mulit msg,waiting_index = %ld", fin_index);
                sched_yield();
            }

            if (next_index <= recv_dataring->capacity)
            {
                rte_memcpy(recv_msg.payload, msg.payload, recv_msg.header->msg_len);
            }
            else
            {
                first_len = ((recv_dataring->capacity - temp_index - 1) << NTP_CONFIG.ntb_packetbits_size) + NTP_CONFIG.datapacket_payload_size;
                rte_memcpy(recv_msg.payload, msg.payload, first_len);
                rte_memcpy((recv_msg.payload + first_len), recv_dataring->start_addr, recv_msg.header->msg_len - first_len);
            }
            // 将所有接收到的消息清空
            for (i = 0; i <= next_index - recv_dataring->cur_index; i++)
            {
                packet_ptr = (void *)(recv_dataring->start_addr + (temp_index << NTP_CONFIG.ntb_packetbits_size));
                temp_index = temp_index + 1 < recv_dataring->capacity ? temp_index + 1 : 0;
                rc = parse_ntpacket(packet_ptr, &msg);
                if (rc == -1 || !msg.header) {
                    continue;
                }
                msg.header->msg_len = 0;
            }
            // global_rece_data += recv_msg.header->msg_len;
            // global_rece_msg += 1;
            // DEBUG("recv_msg counter = %ld,rece_msg_len counter = %ld", global_rece_msg, global_rece_data);
            rc = ntp_shm_send(conn->nts_recv_ring, &recv_msg);
            if (UNLIKELY(rc == -1)) {
                ERR("ntp_shm_send failed");
                return -1;
            }

            recv_dataring->cur_index = next_index + 1 < recv_dataring->capacity ? next_index + 1 : next_index - recv_dataring->capacity + 1;
            
            // DEBUG("recv_msg.payload = %s", recv_msg.payload);
            // DEBUG("receive MULTI_PKG end");
        }
        else if (msg_type == DETECT_PKG)
        {
            INFO("receive DETECT_PKG");
            detect_pkg_handler(ntb_link, msg.header->src_port, msg.header->dst_port, conn); // send the response msg of DETECT_PKG to ntb control msg link (peer node)
            msg.header->msg_len = 0;
            recv_dataring->cur_index = recv_dataring->cur_index + 1 < recv_dataring->capacity ? recv_dataring->cur_index + 1 : 0;
        }
        else if (msg_type == FIN_PKG)
        {
            INFO("receive FIN_PKG");
            recv_msg.header->msg_type = NTP_FIN;
            recv_msg.header->msg_len = msg_len - NTPACKET_HEADER_LEN;

            rc = ntp_shm_send(conn->nts_recv_ring, &recv_msg);
            if (UNLIKELY(rc == -1)) {
                ERR("ntp_shm_send failed");
                return -1;
            }

            conn->state = PASSIVE_CLOSE;
            // DEBUG("conn->state change to passive_close");
            msg.header->msg_len = 0;
            //收到FIN包将state置为CLOSE—CLIENT。遍历ring_list时判断conn->state来close移出node并free
            recv_dataring->cur_index = recv_dataring->cur_index + 1 < recv_dataring->capacity ? recv_dataring->cur_index + 1 : 0;
        }
        else
        {
            ERR("msg_type error");
            msg.header->msg_len = 0;
            recv_dataring->cur_index = recv_dataring->cur_index + 1 < recv_dataring->capacity ? recv_dataring->cur_index + 1 : 0;
        }

        // update the peer-side shadow read index every specific time interval.
        if (partition->recv_packet_counter % IDX_UPDATE_INTERVAL == 0) {
            trans_data_link_cur_index(data_link);
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
    while (!ntb_link->is_stop)
    {
        msg = (struct ntb_ctrl_msg *)(r->start_addr + (r->cur_index << CTRL_NTPACKET_BITS));
        msg_len = msg->header.msg_len;
        // msg_len &= 0x0fff;
        //if no msg,continue
        if (msg_len == 0)
        {
            sched_yield();
            continue;
        }

        DEBUG("receive a ntb_ctrl_msg");
        parser_ctrl_msg_header(ntb_link, msg->header.msg_len);
        //解析接收的包时将src和dst port交换
        src_port = msg->header.dst_port;
        dst_port = msg->header.src_port;
        conn_id = create_conn_id(src_port, dst_port);
        ntb_conn *conn = (ntb_conn *)Get(ntb_link->port2conn, &conn_id);

        //TODO: where peer_read_index is used for Flow Control ??
        ntp_set_peer_read_index(conn->nts_send_ring->ntsring_handle, *(uint64_t *)msg->msg);
        r->cur_index = r->cur_index + 1 < r->capacity ? r->cur_index + 1 : 0;
        msg->header.msg_len = 0;
    }

    return 0;
}


uint16_t ntp_allocate_partition(struct ntb_link_custom *ntb_link) {
    assert(ntb_link);

    uint16_t target_partition_idx = ntb_link->round_robin_idx;
    ntb_link->round_robin_idx++;
    if (ntb_link->round_robin_idx % ntb_link->num_partition == 0) {
        ntb_link->round_robin_idx = 0;
    }

    return target_partition_idx;
}
