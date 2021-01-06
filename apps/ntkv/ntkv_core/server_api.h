#ifndef __SERVER_API_H__
#define __SERVER_API_H__
#include "linked_hashmap.h"
#include "common.h"
#include "batch_api.h"

void cmd_put(int sockfd, map_t map, message_t *in);

void cmd_batch_put(int sockfd, map_t map, batch_message_t *in);

void cmd_batch_get(int sockfd, map_t map, batch_data_t *batch_queue, batch_message_t *in);

void cmd_get(int sockfd, map_t map, message_t *in);

void cmd_remove(int sockfd, map_t map, message_t *in);

void cmd_free(map_t map);

void cmd_destroy(map_t map);

void cmd_len(int sockfd, map_t map);

#endif