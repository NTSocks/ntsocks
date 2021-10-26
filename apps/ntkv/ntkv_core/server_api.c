#include "server_api.h"

void cmd_put(int sockfd, map_t map, message_t *in){

    int n = 0;
    int reply_len = REPLY_SIZE;
    if(in->key_len < 0 && in->value_len < 0){
        int err_no = 1;
        reply_t out;
        out.key_len = 0;
        out.value_len = 0;
        out.err_no = err_no;
        while(1)
        {
            n += write(sockfd, (void*)&out+n, (reply_len-n));
            if(n == reply_len){
                break;
            }
        }
    }
    else{
        void *data = mempool_alloc(mp, in->key_len + in->value_len);
        // memset(data, 0, in->key_len + in->value_len);

        n = 0;
        int len = in->key_len + in->value_len;
        while (true){
            n += read(sockfd, data + n, len - n);
            if (n == len)
                break;
        }

        int err_no = PUT(map, data, data + in->key_len);
        INFO("PUT, errno=%s, key=%s, value=%s, addr=%llu", status_name[err_no], data, data + in->key_len, data);

        reply_t out;
        out.key_len = 0;
        out.value_len = 0;
        out.err_no = err_no;
        n = 0;
        while (1)
        {
            n = write(sockfd, (void*)&out + n, REPLY_SIZE - n);
            if(n == reply_len){
                break;
            }
        }
    }
}

void cmd_batch_put(int sockfd, map_t map, batch_message_t *in){
    INFO("cmd_batch_put, in->data_len=%d", in->data_len);
    void *data = mempool_alloc(mp, in->data_len);
    memset(data, 0, in->data_len);
    int n;
    while (true){
        n = read(sockfd, data, in->data_len);
        if (n == in->data_len)
            break;
    }
    
    INFO("cmd_batch_put read %d bytes, in->data_len=%llu", n, in->data_len);
    // printf("\n");
    // for (int idx = 0; idx < n; ++idx)
    //     printf("[%d,%ld]", idx + sizeof(batch_message_t), *(char*)(data+idx));
    // printf("\n");

    int err_no;
    size_t cur = 0;
    while (cur < in->data_len){
        data_t *d = (data_t*)(data + cur);
        INFO("BATCH PUT, sizeof(data_t)=%llu, key_len=%ld, value_len=%ld", sizeof(data_t), d->key_len, d->value_len);
        err_no = PUT(map, data + cur + sizeof(data_t), data + cur + sizeof(data_t) + d->key_len);
        INFO("BATCH PUT, errno=%s, key=%s, value=%s", status_name[err_no], data + cur + sizeof(data_t), data + cur + sizeof(data_t) + d->key_len);
        if (err_no != MAP_OK && err_no != MAP_REPLACE)
            break;
        cur += (sizeof(data_t) + d->key_len + d->value_len);
    }
    batch_reply_t out;
    out.err_no = err_no;
    out.data_len = 0;
    n = write(sockfd, (void*)&out, sizeof(batch_reply_t));
}

void cmd_batch_get(int sockfd, map_t map, batch_data_t *batch_queue, batch_message_t *in){
    INFO("cmd_batch_get, in->data_len=%d", in->data_len);
    void *data = mempool_alloc(mp, in->data_len);
    memset(data, 0, in->data_len);
    int n;
    while (true){
        n = read(sockfd, data, in->data_len);
        if (n == in->data_len)
            break;
    }
    int err_no;
    size_t cur = 0;
    while (cur < in->data_len){
        data_t *d = (data_t*)(data + cur);
        void *value;
        err_no = GET(map, data + cur + sizeof(data_t), (void**)&value);
        if (err_no == MAP_MISSING)
            break;
        batch_node_t *item = (batch_node_t*) mempool_alloc(mp, sizeof(batch_node_t));
        item->key_len = d->key_len;
        item->value_len = strlen(value) + 1;
        item->data = value - d->key_len;
        INFO("GET, err_name=%s, key=%s, value=%s", status_name[err_no], item->data, value);
        int batch_status = batch_push(batch_queue, item, BATCH_WRITE_GET_REPLY, NULL);
        cur += (sizeof(data_t) + d->key_len);
    }
    batch_write_get_reply(batch_queue, err_no);
    // mempool_free(mp, data);
}

void cmd_get(int sockfd, map_t map, message_t *in){

    int n = 0;
    int reply_len = REPLY_SIZE;
    if(in->key_len < 0){
        int err_no = -1;
        reply_t out;
        out.key_len = 0;
        out.value_len = 0;
        out.err_no = err_no;
        while(1)
        {
            n += write(sockfd, (void*)&out+n, (reply_len-n));
            if(n == reply_len){
                break;
            }
        }
    }
    else
    {
        void *data = mempool_alloc(mp, in->key_len);
        //memset(data, 0, in->key_len);
        n = 0;
        while (true){
            n += read(sockfd, data + n, in->key_len-n);
            if (n == in->key_len)
                break;
        }
        void *value = NULL;
        int err_no = GET(map, data, (void**)&value);
        INFO("GET, err_name=%s, key=%s, value=%s, addr=%llu", status_name[err_no], data, value, value - in->key_len);
        mempool_free(mp, data);

        reply_t resp;
        resp.err_no = err_no;
        resp.key_len = 0;
        resp.value_len = 0;
        if (err_no == MAP_OK){
            resp.value_len = strlen((char*)value) + 1;
        }
        void *out = NULL;
        out = mempool_alloc(mp, REPLY_SIZE + resp.value_len);
        //memset(out, 0, REPLY_SIZE + resp.value_len);
        // DEBUG("[malloc] out data addr is %llu", out);
        memcpy(out, &resp, REPLY_SIZE);
        if (err_no == MAP_OK)
            memcpy(out + REPLY_SIZE, value, resp.value_len);
        
        n = 0;
        int len = REPLY_SIZE + resp.value_len;
        while (1)
        {
            n += write(sockfd, out + n, len - n );
            if( n == len ){
                break;
            }
        }
        mempool_free(mp, out);    
    }
}

void cmd_remove(int sockfd, map_t map, message_t *in){
    // print_hashmap(map);
    // void *data = calloc(in->key_len, sizeof(char));
    void *data = mempool_alloc(mp, in->key_len);
    memset(data, 0, in->key_len);
    // DEBUG("[malloc] in data addr is %llu", data);
    int n;
    while (true){
        n = read(sockfd, data, in->key_len);
        if (n == in->key_len)
            break;
    }

    // free(ptr);
    void *key_addr;
    int err_no = REMOVE(map, data, (void**)&key_addr, NULL);
    if (err_no == MAP_OK)
        INFO("REMOVE, errno=%s, key=%s, key addr=%llu", status_name[err_no], key_addr, key_addr);
    else
        INFO("REMOVE, errno=%s, key=%s", status_name[err_no], data);
    // DEBUG("[free] in data addr is %llu", data);
    mempool_free(mp, data);
    if (err_no == MAP_OK)
        mempool_free(mp, key_addr);
    // print_hashmap(map);

    reply_t out;
    out.key_len = 0;
    out.value_len = 0;
    out.err_no = err_no;
    n = write(sockfd, (void*)&out, REPLY_SIZE);
}

void cmd_free(map_t map){
    // free key-value address
    // print_hashmap(map);
    // Iterator *it = new_iterator(map);
    // while (has_next(it)){
    //     it = next(it);
    //     // DEBUG("free start, [%s,%s], addr=%llu", it->entry->key, it->entry->value, it->entry->key);
    //     INFO("free, key=%s, value=%s, addr=%llu", it->entry->key, it->entry->value, it->entry->key);
    //     mempool_free(mp, it->entry->key);
    // }
    // free_iterator(it);
    // free data node

    FREE(map);
    // print_hashmap(map);
    // DEBUG("free hashmap");
}

void cmd_destroy(map_t map){
    DESTROY(map);
}

void cmd_len(int sockfd, map_t map){
    int len = LEN(map);
    // DEBUG("LEN, len is %d", len);
    INFO("LEN, len=%d", len);

    reply_t out;
    out.err_no = MAP_OK;
    out.key_len = len;
    out.value_len = 0;
    write(sockfd, (void*)&out, REPLY_SIZE);
}