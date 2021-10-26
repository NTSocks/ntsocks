#include "client_api.h"

reply_t map_put(int sockfd, item_t *key, item_t *value){
    DEBUG("map_put, key:%s, value:%s", key->data, value->data);
    message_t msg;
    msg.cmd = CMD_PUT;
    msg.key_len = key->len;
    msg.value_len = value->len;
    // void *out = malloc(sizeof(message_t) + key->len + value->len);
    void *out = mempool_alloc(mp, sizeof(message_t) + key->len + value->len);
    memset(out, 0, sizeof(message_t) + key->len + value->len);
    memcpy(out, &msg, sizeof(message_t));
    memcpy(out + sizeof(message_t), key->data, key->len);
    memcpy(out + sizeof(message_t) + key->len, value->data, value->len);
    int n = write(sockfd, out, sizeof(message_t) + key->len + value->len);
    // free(out);
    mempool_free(mp, out);

    reply_t in;
    while (true){
        n = read(sockfd, &in, sizeof(reply_t));
        if (n == sizeof(reply_t))
            break;
    }
    return in;
}

reply_t map_get(int sockfd,  item_t *key, item_t **value){
    DEBUG("map_get, key:%s", key->data);
    message_t msg;
    msg.cmd = CMD_GET;
    msg.key_len = key->len;
    msg.value_len = 0;
    // void *out = malloc(sizeof(message_t) + key->len);
    void *out = mempool_alloc(mp, sizeof(message_t) + key->len);
    memset(out, 0, sizeof(message_t) + key->len);
    memcpy(out, &msg, sizeof(message_t));
    memcpy(out + sizeof(message_t), key->data, key->len);
    int n = write(sockfd, out, sizeof(message_t) + key->len);
    // free(out);
    mempool_free(mp, out);

    reply_t in;
    
    while (true){
        n = read(sockfd, &in, sizeof(reply_t));
        if (n == sizeof(reply_t))
            break;
    }
    if (in.err_no == MAP_OK){
        // DEBUG("map get, key len is %llu, value len is %llu", in.key_len, in.value_len);
        (*value)->len = in.value_len;
        (*value)->data = mempool_alloc(mp, in.value_len);
        // memset((*value)->data, 0, in.value_len);
        while (true){
            n = read(sockfd, (*value)->data, in.value_len);
            if (n == in.value_len)
                break;
        }
    }
    return in;
}

int map_remove(int sockfd, item_t *key){
    DEBUG("map_remove, key:%s", key->data);
    message_t msg;
    msg.cmd = CMD_REMOVE;
    msg.key_len = key->len;
    msg.value_len = 0;
    // void *out = malloc(sizeof(message_t) + key->len);
    void *out = mempool_alloc(mp, sizeof(message_t) + key->len);
    memset(out, 0, sizeof(message_t) + key->len);
    memcpy(out, &msg, sizeof(message_t));
    memcpy(out + sizeof(message_t), key->data, key->len);
    int n = write(sockfd, out, sizeof(message_t) + key->len);
    // free(out);
    mempool_free(mp, out);

    reply_t in;
    while (true){
        n = read(sockfd, &in, sizeof(reply_t));
        if (n == sizeof(reply_t))
            break;
    }
    return in.err_no;
}

void map_free(int sockfd){
    message_t msg;
    msg.cmd = CMD_FREE;
    msg.key_len = 0;
    msg.value_len = 0;
    write(sockfd, &msg, sizeof(message_t));
}

void map_destroy(int sockfd){
    message_t msg;
    msg.cmd = CMD_DESTROY;
    msg.key_len = 0;
    msg.value_len = 0;
    write(sockfd, &msg, sizeof(message_t));
}

int map_size(int sockfd){
    message_t msg;
    msg.cmd = CMD_LEN;
    msg.key_len = 0;
    msg.value_len = 0;
    int n = write(sockfd, &msg, sizeof(message_t));
    
    reply_t in;
    while (true){
        n = read(sockfd, &in, sizeof(reply_t));
        if (n == sizeof(reply_t))
            break;
    }
    DEBUG("size, response errno is %s", status_name[in.err_no]);
    return in.key_len;
}
