#ifndef __CLIENT_API_H__
#define __CLIENT_API_H__
#include "common.h"
#include "mempool.h"
// DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

mempool_t *mp;

reply_t map_put(int sockfd, item_t *key, item_t *value);

reply_t map_get(int sockfd,  item_t *key, item_t **value);

int map_remove(int sockfd, item_t *key);

void map_free(int sockfd);

void map_destroy(int sockfd);

int map_size(int sockfd);

#endif