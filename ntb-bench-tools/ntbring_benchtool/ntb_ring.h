#ifndef __NTB_RING_H__
#define __NTB_RING_H__

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define KB (1 << 10)
#define MB (1 << 20)
#define GB (1 << 30)

#define NTB_MAX_BUF 512 * MB

struct ring_item
{
    uint64_t len;
    char *data;
};

struct ntb_link
{
    struct rte_rawdev *dev;
    struct ntb_hw *hw;

    // 指向本地NTB的基地址
    uint8_t *local_ptr;
    // 指向对端NTB PCIe设备的基地址
    uint8_t *remote_ptr;

    // 当前本地ring的基地址
    uint8_t *cur_local_ptr;
    // 当前对端ring的基地址
    uint8_t *cur_remote_ptr;

    struct ntb_ring *local_ring;
    struct ntb_ring *remote_ring;

    uint64_t local_cus_ptr; // 本地消费者指针
    uint64_t remote_pd_ptr; // 对端生产者指针

    int (*ring_enqueue)(struct ntb_link *link, struct ring_item *in_item);
    int (*ring_dequeue)(struct ntb_link *link, struct ring_item *out_item);
};

// 每个ring的header大小
#define HEADER_OF_RING sizeof(struct ring_t)

struct ring_t
{
    volatile uint64_t cus_ptr; // 消费指针, 可指向本地或远端
    volatile uint64_t pd_ptr;  // 生产指针, 可指向本地或远端
} __attribute__((packed));

// 每个元素头部8字节用于标明每个元素大小
#define HEADER_OF_ELE sizeof(uint64_t)

/**
 * 五种方式：
 * 1. local_ring中cus_ptr和pd_ptr均表示本地
 * 2. local_ring中cus_ptr表示本地消费指针，pd_ptr表示对端生产指针
 * 3. local_ring中cus_ptr表示对端消费指针，pd_ptr表示本地生产指针
 * 4. local_ring中cus_ptr和pd_ptr均表示对端
 * 5. local_ring中cus_ptr表示本地消费指针，remote_ring中
 *      pd_ptr表示对端生产指针，本地留出32 bits同步远程消费指针
 */

enum NTB_RING_TYPE
{
    // 额外开销：enqueue (1 remote read & 1 remote write)
    LOCAL_CUS_LOCAL_PD = 1,
    // 额外开销：enqueue (1 remote read), dequeue (1 remote read)
    LOCAL_CUS_REMOTE_PD,
    // 额外开销：enqueue (1 remote write), dequeue (1 remote write)
    REMOTE_CUS_LOCAL_PD,
    // 额外开销：dequeue(1 remote read & 1 remote write)
    REMOTE_CUS_REMOTE_PD,
    LOCAL_CUS_OPTIMAL
};

struct ntb_ring
{

    int num_of_ele;
    int size_of_ele;

    uint8_t *buf; // required
    struct ring_t *ring;
};

static struct ntb_ring *
ring_create(uint8_t *ptr, int num_of_ele, int size_of_ele,
            enum NTB_RING_TYPE ring_type, uint8_t *base_ptr);

void ring_init(struct ntb_link *link, int num_of_ele,
               int size_of_ele, enum NTB_RING_TYPE ring_type);

int ring_enqueue_1(struct ntb_link *link, struct ring_item *in_item);
int ring_enqueue_2(struct ntb_link *link, struct ring_item *in_item);
int ring_enqueue_3(struct ntb_link *link, struct ring_item *in_item);
int ring_enqueue_4(struct ntb_link *link, struct ring_item *in_item);

int ring_dequeue_1(struct ntb_link *link, struct ring_item *out_item);
int ring_dequeue_2(struct ntb_link *link, struct ring_item *out_item);
int ring_dequeue_3(struct ntb_link *link, struct ring_item *out_item);
int ring_dequeue_4(struct ntb_link *link, struct ring_item *out_item);

bool out_of_memory(uint8_t *ptr, int num_of_ele, int size_of_ele, uint8_t *base_ptr);

void ring_dump(struct ntb_link *link);

#endif