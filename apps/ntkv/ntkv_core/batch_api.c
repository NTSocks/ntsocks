
#include "batch_api.h"

batch_data_t* batch_init(mempool_t *mempool, int sockfd, d_len queue_size){
    assert(mempool);
    assert(sockfd > 0);
    batch_data_t *batch_queue = (batch_data_t*) mempool_alloc(mempool, sizeof(batch_data_t));
    batch_queue->mempool = mempool;
    batch_queue->sockfd = sockfd;
    batch_queue->max_queue_size = queue_size > 0 ? queue_size : 1000;
    batch_queue->queue_size = 0;
    batch_queue->data_size = 0;
    TAILQ_INIT(&batch_queue->queue_head);
    return batch_queue;
}

int batch_push(batch_data_t *batch_queue, batch_node_t *item, int op, batch_reply_t *out){
    assert(batch_queue);
    assert(item);
    TAILQ_INSERT_TAIL(&batch_queue->queue_head, item, data_link);
    batch_queue->queue_size++;
    batch_queue->data_size += (sizeof(data_t) + item->key_len + item->value_len);
    if (batch_queue->queue_size >= batch_queue->max_queue_size){
        if (op == BATCH_PUT)
            *out = batch_write_put(batch_queue);
        else if (op == BATCH_GET)
            *out = batch_write_get(batch_queue);
        else if (op == BATCH_WRITE_GET_REPLY)
            batch_write_get_reply(batch_queue, 0);
        return BATCH_FULL;
    }
    return BATCH_OK;
}

batch_reply_t batch_write_get(batch_data_t *batch_queue){
    assert(batch_queue);
    batch_reply_t in;
    if (batch_queue->queue_size <= 0){
        in.err_no = BATCH_EMPTY;
        in.data_len = 0;
        return in;
    }
    batch_write(batch_queue, CMD_BATCH_GET);

    int n;
    while (true){
        n = read(batch_queue->sockfd, &in, sizeof(batch_reply_t));
        if (n == sizeof(batch_reply_t))
            break;
    }
    if (in.data_len > 0){
        void *get_data = mempool_alloc(batch_queue->mempool, in.data_len);
        while (true){
            n = read(batch_queue->sockfd, get_data, in.data_len);
            if (n == in.data_len)
                break;
        }
        size_t cur = 0;
        while (cur < in.data_len){
            data_t *d = (data_t*)(get_data + cur);
            INFO("batch_write_get, key = %s, value = %s", get_data + cur + sizeof(data_t), get_data + cur + sizeof(data_t) + d->key_len);
            cur += (sizeof(data_t) + d->key_len + d->value_len);
        }
    }
    return in;
}

batch_reply_t batch_write_put(batch_data_t *batch_queue){
    assert(batch_queue);
    batch_reply_t in;
    if (batch_queue->queue_size <= 0){
        in.err_no = BATCH_EMPTY;
        return in;
    }
    batch_write(batch_queue, CMD_BATCH_PUT);

    int n = 0;
    while(true){
        n = read(batch_queue->sockfd, &in, sizeof(batch_reply_t));
        if (n == sizeof(batch_reply_t))
            break;
    }
    INFO("batch_write_put, read %d bytes from server", n);
    return in;
}

void batch_write_get_reply(batch_data_t *batch_queue, int err_no){
    assert(batch_queue);
    if (batch_queue->queue_size <= 0)
        return;
    batch_reply_t reply;
    reply.err_no = err_no;
    reply.data_len = batch_queue->data_size;
    void *out = mempool_alloc(batch_queue->mempool, sizeof(batch_reply_t) + reply.data_len);
    memset(out, 0, sizeof(batch_reply_t) + reply.data_len);
    size_t cur = 0;
    memcpy(out + cur, &reply, sizeof(batch_reply_t));
    cur += sizeof(batch_reply_t);
    batch_node_t *item = NULL;
    while (batch_queue->queue_size > 0){
        item = batch_pop(batch_queue);
        data_t d;
        d.key_len = item->key_len;
        d.value_len = item->value_len;
        memcpy(out + cur, &d, sizeof(data_t));
        cur += sizeof(data_t);
        INFO("key=%s, value=%s", item->data, item->data + d.key_len);
        memcpy(out + cur, item->data, d.key_len + d.value_len);
        cur += (d.key_len + d.value_len);
    }
    INFO("msg.cmd=%d, cur=%llu, msg.data_len=%llu", err_no, cur, reply.data_len);
    int n = write(batch_queue->sockfd, out, sizeof(batch_reply_t) + reply.data_len);
    INFO("batch write get reply %d bytes", n);
    // mempool_free(batch_queue->mempool, out);
}

static void batch_write(batch_data_t *batch_queue, int cmd){
    if (batch_queue->queue_size <= 0)
        return;
    batch_message_t msg;
    msg.cmd = cmd;
    msg.data_len = batch_queue->data_size;
    void *out = mempool_alloc(batch_queue->mempool, sizeof(batch_message_t) + msg.data_len);
    memset(out, 0, sizeof(batch_message_t) + msg.data_len);
    size_t cur = 0;
    memcpy(out + cur, &msg, sizeof(batch_message_t));
    cur += sizeof(batch_message_t);
    batch_node_t *item = NULL;
    while (batch_queue->queue_size > 0){
        item = batch_pop(batch_queue);
        data_t d;
        d.key_len = item->key_len;
        d.value_len = item->value_len;
        memcpy(out + cur, &d, sizeof(data_t));
        cur += sizeof(data_t);
        INFO("key=%s, value=%s", item->data, item->data + d.key_len);
        memcpy(out + cur, item->data, d.key_len + d.value_len);
        cur += (d.key_len + d.value_len);
    }
    INFO("msg.cmd=%d, cur=%llu, msg.data_len=%llu", cmd, cur, msg.data_len);
    // printf("\n");
    // for (int idx = 0; idx < cur; ++idx)
    //     printf("[%d,%ld]", idx, *(char*)(out+idx));
    // printf("\n");
    int n = write(batch_queue->sockfd, out, sizeof(batch_message_t) + msg.data_len);
    INFO("batch write %d bytes", n);
    // mempool_free(batch_queue->mempool, out);
}

batch_node_t* batch_pop(batch_data_t *batch_queue){
    assert(batch_queue);
    batch_node_t *data = NULL;
    if (batch_queue->queue_size <= 0)
        return data;
    data = TAILQ_FIRST(&batch_queue->queue_head);
    if (!data){
        fprintf(stderr, "TAILQ_FIRST ERR");
        return data;
    }
    TAILQ_REMOVE(&batch_queue->queue_head, data, data_link);
    batch_queue->queue_size--;
    batch_queue->data_size -= (sizeof(data_t) + data->key_len + data->value_len);
    return data;
}

void batch_destroy(batch_data_t *batch_queue){
    assert(batch_queue);
    mempool_free(batch_queue->mempool, batch_queue);
}
