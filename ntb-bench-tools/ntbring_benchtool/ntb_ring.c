#include <rte_io.h>
#include <rte_memcpy.h>

#include "ntb_ring.h"
#include "ntb_log.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);

bool out_of_memory(uint8_t *ptr, int num_of_ele, int size_of_ele, uint8_t *base_ptr)
{
    assert(ptr);
    assert(num_of_ele > 0);
    assert(size_of_ele > 0);
    assert(base_ptr);

    if (ptr + HEADER_OF_RING + num_of_ele * (size_of_ele + HEADER_OF_ELE) > base_ptr + NTB_MAX_BUF)
        return true;

    return false;
}

static struct ntb_ring *ring_create(uint8_t *ptr, int num_of_ele, int size_of_ele, enum NTB_RING_TYPE ring_type, uint8_t *base_ptr)
{
    assert(ptr);
    assert(num_of_ele > 0);
    assert(size_of_ele > 0);
    assert(base_ptr);

    DEBUG("ring create begin, ptr=%p, num_of_ele=%d, size_of_ele=%d", ptr, num_of_ele, size_of_ele);
    struct ntb_ring *ring = (struct ntb_ring *)malloc(sizeof(struct ntb_ring));

    ring->num_of_ele = num_of_ele;
    ring->size_of_ele = size_of_ele;

    if (out_of_memory(ptr, num_of_ele, size_of_ele, base_ptr))
    {
        ERR("ring_init out of NTB memory");
        return NULL;
    }
    ring->buf = (uint8_t *)(ptr + HEADER_OF_RING);

    ring->ring = (struct ring_t *)ptr;
    ring->ring->cus_ptr = ring->ring->pd_ptr = 0;

    return ring;
}

void ring_init(struct ntb_link *link, int num_of_ele, int size_of_ele, enum NTB_RING_TYPE ring_type)
{
    assert(link);
    assert(num_of_ele > 0);
    assert(size_of_ele > 0);

    DEBUG("ring init begin. num_of_ele=%d, size_of_ele=%d", num_of_ele, size_of_ele);

    link->local_ring = ring_create(link->cur_local_ptr, num_of_ele, size_of_ele, ring_type, link->local_ptr);
    link->cur_local_ptr = link->local_ring->buf + num_of_ele * (HEADER_OF_ELE + size_of_ele);

    link->remote_ring = ring_create(link->cur_remote_ptr, num_of_ele, size_of_ele, ring_type, link->remote_ptr);
    link->cur_remote_ptr = link->remote_ring->buf + num_of_ele * (HEADER_OF_ELE + size_of_ele);

    switch (ring_type)
    {

    case LOCAL_CUS_LOCAL_PD:
    {
        link->ring_enqueue = ring_enqueue_1;
        link->ring_dequeue = ring_dequeue_1;

        break;
    }

    case LOCAL_CUS_REMOTE_PD:
    {
        link->ring_enqueue = ring_enqueue_2;
        link->ring_dequeue = ring_dequeue_2;
        break;
    }

    case REMOTE_CUS_LOCAL_PD:
    {
        link->ring_enqueue = ring_enqueue_3;
        link->ring_dequeue = ring_dequeue_3;
        break;
    }

    case REMOTE_CUS_REMOTE_PD:
    {
        link->ring_enqueue = ring_enqueue_4;
        link->ring_dequeue = ring_dequeue_4;
        break;
    }

    case LOCAL_CUS_OPTIMAL:
    {

        break;
    }

    default:
    {
        ERR("NO such ring_type[%d]", ring_type);
        break;
    }
    }
}

int ring_enqueue_1(struct ntb_link *link, struct ring_item *in_item)
{
    assert(link);
    assert(in_item);
    assert(in_item->data);
    assert(in_item->len > 0);

    /**
     * local cus ptr and local pd ptr
     * 1. 比较remote ring的cus ptr和remote_pd_ptr, 判断remote ring是否为满
     * 2. 获取remote ring的ring buf的基地址将数据写入对端
     * 3. 更新remote_pd_ptr和remote ring的pd ptr
     */

    if (in_item->len <= 0 || in_item->len > link->remote_ring->size_of_ele)
    {
        ERR("ring_enqueue, length of in_item[%ld] > size of ring's element[%d]", in_item->len, link->remote_ring->size_of_ele);
        return -1;
    }

    uint64_t next_idx = (link->remote_pd_ptr + 1) % link->remote_ring->num_of_ele;
    uint64_t cus_ptr = link->remote_ring->ring->cus_ptr;
    if (next_idx == cus_ptr)
    {
        INFO("ring_enqueue, remote ring is full. link->remote_ring->ring->cus_ptr=%lu, link->remote_pd_ptr=%lu", link->remote_ring->ring->cus_ptr, link->remote_pd_ptr);
        return 1;
    }

    // remote write写入数据到对端
    uint8_t *ptr = link->remote_ring->buf + link->remote_pd_ptr * (link->remote_ring->size_of_ele + HEADER_OF_ELE);

    // 先写data, 再写len. 保证读取len之前data写进去了
    rte_memcpy(ptr, in_item->data, in_item->len);
    *((uint64_t *)(ptr + link->remote_ring->size_of_ele)) = in_item->len;

    link->remote_pd_ptr = next_idx;
    link->remote_ring->ring->pd_ptr = next_idx;

    // mem barrier
    rte_wmb();

    return 0;
}

int ring_enqueue_2(struct ntb_link *link, struct ring_item *in_item)
{
    assert(link);
    assert(in_item);
    assert(in_item->data);
    assert(in_item->len > 0);

    /**
     * local cus ptr and remote pd ptr
     * 1. 比较remote ring的cus ptr和local ring的pd ptr，判断remote ring是否满了
     * 2. 获取remote ring的ring buf的基地址将数据写入对端
     * 3. 更新local ring的pd ptr
     */

    if (in_item->len <= 0 || in_item->len > link->remote_ring->size_of_ele)
    {
        ERR("ring_enqueue, length of in_item[%ld] > size of ring's element[%d]", in_item->len, link->remote_ring->size_of_ele);
        return -1;
    }

    uint64_t pd_ptr = link->local_ring->ring->pd_ptr;
    uint64_t next_idx = (pd_ptr + 1) % link->remote_ring->num_of_ele;

    if (next_idx == link->remote_ring->ring->cus_ptr)
    {
        INFO("ring_enqueue, remote ring is full. local_ring->ring->cus_ptr=%lu, link->local_ring->ring->pd_ptr=%lu, remote_ring->ring->cus_ptr=%lu, remote_ring->ring->pd_ptr=%lu",
             link->local_ring->ring->cus_ptr, link->local_ring->ring->pd_ptr, link->remote_ring->ring->cus_ptr, link->remote_ring->ring->pd_ptr);
        return 1;
    }

    uint8_t *ptr = link->remote_ring->buf + pd_ptr * (link->remote_ring->size_of_ele + HEADER_OF_ELE);

    rte_memcpy(ptr, in_item->data, in_item->len);
    *((uint64_t *)(ptr + link->remote_ring->size_of_ele)) = in_item->len;

    link->local_ring->ring->pd_ptr = next_idx;

    return 0;
}

int ring_enqueue_3(struct ntb_link *link, struct ring_item *in_item)
{
    assert(link);
    assert(in_item);
    assert(in_item->data);
    assert(in_item->len > 0);

    /**
     * remote cus ptr and local pd ptr
     * 1. 比较local ring的cus ptr和remote_pd_ptr, 判断remote ring是否为满
     * 2. 获取remote ring的ring buf基地址将数据写入对端
     * 3. 更新remote_pd_ptr和remote ring的pd ptr
     */

    if (in_item->len <= 0 || in_item->len > link->remote_ring->size_of_ele)
    {
        ERR("ring_enqueue, length of in_item[%ld] > size of ring's element[%d]", in_item->len, link->remote_ring->size_of_ele);
        return -1;
    }

    uint64_t next_idx = (link->remote_pd_ptr + 1) % link->remote_ring->num_of_ele;
    if (next_idx == link->local_ring->ring->cus_ptr)
    {
        INFO("ring_enqueue, remote ring is full. local_ring->ring->cus_ptr=%lu, link->remote_pd_ptr=%lu", link->local_ring->ring->cus_ptr, link->remote_pd_ptr);
        return 1;
    }

    uint8_t *ptr = link->remote_ring->buf + link->remote_pd_ptr * (link->remote_ring->size_of_ele + HEADER_OF_ELE);

    rte_memcpy(ptr, in_item->data, in_item->len);
    *((uint64_t *)(ptr + link->remote_ring->size_of_ele)) = in_item->len;

    link->remote_pd_ptr = next_idx;
    // rte_memcpy(&link->remote_ring->ring->pd_ptr, &next_idx, sizeof(uint64_t));
    link->remote_ring->ring->pd_ptr = next_idx;

    // mem barrier
    rte_wmb();

    return 0;
}

int ring_enqueue_4(struct ntb_link *link, struct ring_item *in_item)
{
    assert(link);
    assert(in_item);
    assert(in_item->data);
    assert(in_item->len > 0);

    /**
     * remote cus ptr and remote pd ptr
     * 1. 比较local ring的cus ptr和pd ptr, 判断remote ring是否为满
     * 2. 获取remote ring的ring buf基地址将数据写入对端
     * 3. 更新local ring的pd ptr
     */

    struct ntb_ring *local_ring = link->local_ring;

    if (in_item->len <= 0 || in_item->len > local_ring->size_of_ele)
    {
        ERR("ring_enqueue, length of in_item[%ld] > size of ring's element[%d]", in_item->len, local_ring->size_of_ele);
        return -1;
    }

    uint64_t pd_ptr = local_ring->ring->pd_ptr;
    uint64_t next_idx = (pd_ptr + 1) % link->remote_ring->num_of_ele;
    if (next_idx == local_ring->ring->cus_ptr)
    {
        INFO("ring_enqueue, remote ring is full. local_ring->ring->cus_ptr=%lu, local_ring->ring->pd_ptr=%lu", local_ring->ring->cus_ptr, local_ring->ring->pd_ptr);
        return 1;
    }

    // remote write写入数据到对端
    uint8_t *ptr = link->remote_ring->buf + pd_ptr * (link->remote_ring->size_of_ele + HEADER_OF_ELE);

    rte_memcpy(ptr, in_item->data, in_item->len);
    *((uint64_t *)(ptr + link->remote_ring->size_of_ele)) = in_item->len;

    link->local_ring->ring->pd_ptr = next_idx;

    return 0;
}

int ring_dequeue_1(struct ntb_link *link, struct ring_item *out_item)
{
    assert(link);
    assert(out_item);

    /**
     * local cus ptr and local pd ptr
     * 1. 比较local ring的cus ptr和local ring的pd ptr, 判断local ring是否为空
     * 2. 获取local ring的ring buf基地址, 将数据从buf中读取出来
     * 3. 更新local ring的cus ptr
     */
    uint64_t cus_ptr = link->local_ring->ring->cus_ptr;
    uint64_t pd_ptr = link->local_ring->ring->pd_ptr;
    if (cus_ptr == pd_ptr)
    {
        INFO("ring_dequeue, local ring is empty. link->local_ring->ring->cus_ptr=%lu, link->local_ring->ring->pd_ptr=%lu", link->local_ring->ring->cus_ptr, link->local_ring->ring->pd_ptr);
        return 1;
    }

    uint64_t next_idx = (cus_ptr + 1) % link->local_ring->num_of_ele;

    uint8_t *ptr = link->local_ring->buf + cus_ptr * (HEADER_OF_ELE + link->local_ring->size_of_ele);

    out_item->len = *((uint64_t *)(ptr + link->local_ring->size_of_ele));
    if (out_item->len <= 0 || out_item->len > link->local_ring->size_of_ele)
    {
        INFO("ring_dequeue, out_item->len=%lu, *((uint64_t*)(ptr + link->local_ring->size_of_ele))=%lu", out_item->len, *((uint64_t *)(ptr + link->local_ring->size_of_ele)));
        return -1;
    }
    rte_memcpy(out_item->data, ptr, out_item->len);
    *((uint64_t *)(ptr + link->local_ring->size_of_ele)) = 0;

    link->local_ring->ring->cus_ptr = next_idx;

    return 0;
}

int ring_dequeue_2(struct ntb_link *link, struct ring_item *out_item)
{
    assert(link);
    assert(out_item);

    /**
     * local cus ptr and remote pd ptr
     * 1. 比较local ring的cus ptr和remote ring的pd ptr, 判断local ring是否为空
     * 2. 获取local ring的ring buf基地址, 将数据从buf中读取出来
     * 3. 更新local ring的cus ptr
     */
    // ERR("link->remote_ring->ring->pd_ptr=%u", link->remote_ring->ring->pd_ptr);
    if (link->local_ring->ring->cus_ptr == link->remote_ring->ring->pd_ptr)
    {
        INFO("ring_dequeue, local ring is empty.");
        return 1;
    }

    uint64_t next_idx = (link->local_ring->ring->cus_ptr + 1) % link->local_ring->num_of_ele;

    uint8_t *ptr = link->local_ring->buf + link->local_ring->ring->cus_ptr * (HEADER_OF_ELE + link->local_ring->size_of_ele);
    out_item->len = *((uint64_t *)(ptr + link->local_ring->size_of_ele));
    if (out_item->len <= 0 || out_item->len > link->local_ring->size_of_ele)
    {
        INFO("ring_dequeue, out_item->len=%lu, *((uint64_t*)(ptr + link->local_ring->size_of_ele))=%lu", out_item->len, *((uint64_t *)(ptr + link->local_ring->size_of_ele)));
        return -1;
    }
    rte_memcpy(out_item->data, ptr, out_item->len);
    *((uint64_t *)(ptr + link->local_ring->size_of_ele)) = 0;

    link->local_ring->ring->cus_ptr = next_idx;

    return 0;
}

int ring_dequeue_3(struct ntb_link *link, struct ring_item *out_item)
{
    assert(link);
    assert(out_item);

    /**
     * remote cus ptr and local pd ptr
     * 1. 比较local_cus_ptr和local ring的pd ptr, 判断local ring是否为空
     * 2. 获取local ring的ring buf基地址将数据读取出来
     * 3. 更新local_cus_ptr和remote ring的cus ptr
     */

    if (link->local_cus_ptr == link->local_ring->ring->pd_ptr)
    {
        INFO("ring_dequeue, local ring is empty. link->local_cus_ptr=%lu, link->local_ring->ring->pd_ptr=%lu", link->local_cus_ptr, link->local_ring->ring->pd_ptr);
        return 1;
    }

    uint64_t next_idx = (link->local_cus_ptr + 1) % link->local_ring->num_of_ele;
    uint8_t *ptr = link->local_ring->buf + link->local_cus_ptr * (HEADER_OF_ELE + link->local_ring->size_of_ele);

    out_item->len = *((uint64_t *)(ptr + link->local_ring->size_of_ele));
    if (out_item->len <= 0 || out_item->len > link->local_ring->size_of_ele)
    {
        INFO("ring_dequeue, out_item->len=%lu, *((uint64_t*)(ptr + link->local_ring->size_of_ele))=%lu", out_item->len, *((uint64_t *)(ptr + link->local_ring->size_of_ele)));
        return -1;
    }
    rte_memcpy(out_item->data, ptr, out_item->len);
    *((uint64_t *)(ptr + link->local_ring->size_of_ele)) = 0;

    link->local_cus_ptr = next_idx;
    // rte_memcpy(&link->remote_ring->ring->cus_ptr, &next_idx, sizeof(uint64_t));
    link->remote_ring->ring->cus_ptr = next_idx;

    // mem barrier
    rte_wmb();

    return 0;
}

int ring_dequeue_4(struct ntb_link *link, struct ring_item *out_item)
{
    assert(link);
    assert(out_item);

    /**
     * remote cus ptr and remote pd ptr
     * 1. 比较local_cus_ptr和remote ring的pd ptr
     * 2. 获取local ring的ring buf基地址将数据读取出来
     * 3. 更新local_cus_ptr和remote ring的cus ptr
     */
    struct ntb_ring *remote_ring = link->remote_ring;

    if (link->local_cus_ptr == remote_ring->ring->pd_ptr)
    {
        INFO("ring_dequeue, local ring is empty. link->local_cus_ptr=%lu, link->local_ring->ring->pd_ptr=%lu, remote_ring->ring->pd_ptr=%lu",
             link->local_cus_ptr, link->local_ring->ring->pd_ptr, remote_ring->ring->pd_ptr);
        return 1;
    }

    uint64_t next_idx = (link->local_cus_ptr + 1) % link->local_ring->num_of_ele;
    uint8_t *ptr = link->local_ring->buf + link->local_cus_ptr * (HEADER_OF_ELE + remote_ring->size_of_ele);

    out_item->len = *((uint64_t *)(ptr + link->local_ring->size_of_ele));
    if (out_item->len <= 0 || out_item->len > link->local_ring->size_of_ele)
    {
        INFO("ring_dequeue, out_item->len=%lu, *((uint64_t*)(ptr + link->local_ring->size_of_ele))=%lu", out_item->len, *((uint64_t *)(ptr + link->local_ring->size_of_ele)));
        return -1;
    }

    rte_memcpy(out_item->data, ptr, out_item->len);
    *((uint64_t *)(ptr + remote_ring->size_of_ele)) = 0;

    link->local_cus_ptr = next_idx;
    link->remote_ring->ring->cus_ptr = next_idx;

    // mem barrier
    rte_wmb();

    return 0;
}
