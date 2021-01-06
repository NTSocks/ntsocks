#include <stdlib.h>
#include <stdio.h>
#include "../linked_hashmap.h"
#include "../kv_errno.h"
#include "../mempool.h"

#define KEY_CONUT 10
#define KEY_PREFIX "mykey"
#define VALUE_PREFIX "myvalue"
#define KEY_MAX_LENGTH 256

// char* status_name[5] = {
//     "MAP_OK",
//     "MAP_REPLACE",
//     "MAP_MISSING",
//     "MAP_OMEM",
//     "MAP_ERR"
// };

struct map_data{
    char key[KEY_MAX_LENGTH];
    char value[KEY_MAX_LENGTH];
}map_data;

// mempool_t *mp;

int main(int argc, char *argv[]){
    mp = mempool_init(0.2 * GB, 2 * GB, 0);
    map_t my_map = new_hashmap(0, NULL);

    int i, err_no;
    char key_string[KEY_MAX_LENGTH];

    struct map_data *data;
    void *value;
    for (i = 0; i < KEY_CONUT; i++){
        // data = (struct map_data*)malloc(sizeof(struct map_data));
        data = (struct map_data*)mempool_alloc(mp, sizeof(struct map_data));
        memset(data, 0, sizeof(struct map_data));
        snprintf(data->key, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, i);
        snprintf(data->value, KEY_MAX_LENGTH, "%s-%d", VALUE_PREFIX, i);
        
        err_no = PUT(my_map, (any_t)data->key, (any_t)data->value);
        if(err_no != MAP_OK)
            fprintf(stderr, "line:%d, hashmap put failed, err_no is %d\n", __LINE__ ,err_no);
        // printf("[map put]key=%s, value=%d\n", data->key, data->value);
    }
    printf("hashmap size is %d\n", LEN(my_map));

    for (i = 0; i < KEY_CONUT; i++){
        snprintf(key_string, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, i);
        // printf("[map get start]key=%s\n", key_string);
        err_no = GET(my_map, (any_t)key_string, (any_t*)&value);
        if(err_no != MAP_OK)
            fprintf(stderr, "line:%d, hashmap get failed, err_no is %d\n", __LINE__, err_no);
        printf("[map get end]key=%s, value=%s\n", value - KEY_MAX_LENGTH, value);
    }
    snprintf(key_string, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, KEY_CONUT);
    err_no = GET(my_map, (any_t)key_string, (any_t*)&data);
    printf("GET NONE ELE, err_string=%s\n", status_name[err_no]);
    if(err_no != MAP_MISSING){
        fprintf(stderr, "line:%d, map get failed, err_no is %d\n", __LINE__, err_no);
        return 1;
    }
    void *key_addr, *value_addr;
    err_no = REMOVE(my_map, (any_t)key_string, (void**)&key_addr, (void**)&value_addr);
    printf("REMOVE NONE ELE, err_string=%s\n", status_name[err_no]);
    snprintf(key_string, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, 0);
    err_no = REMOVE(my_map, (any_t)key_string, (void**)&key_addr, (void**)&value_addr);
    printf("REMOVE 0 ELE, err_string=%s\n", status_name[err_no]);
    print_mempool(mp);
    printf("key_addr is %llu\n", key_addr);
    mempool_free(mp, key_addr);
    printf("hashmap size is %d\n", LEN(my_map));
    print_mempool(mp);
    for (i = 1; i < KEY_CONUT; i++){
        snprintf(key_string, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, i);
        
        err_no = GET(my_map, (any_t)key_string, (any_t*)&value);
        if(err_no != MAP_OK)
            fprintf(stderr, "line:%d, hashmap get failed, err_no is %d\n", __LINE__, err_no);
        err_no = REMOVE(my_map, (any_t)key_string, (void**)&key_addr, (void**)&value_addr);
        if(err_no != MAP_OK){
            fprintf(stderr, "line:%d, hashmap remove failed, err_no is %d\n", __LINE__, err_no);
        }
        // print_mempool(mp);
        printf("key_addr is %llu\n", key_addr);
        mempool_free(mp, key_addr);
    }
    printf("hashmap size is %d\n", LEN(my_map));
    FREE(my_map);
    DESTROY(my_map);
    print_mempool(mp);
    // mempool_clear(mp);
    // print_mempool(mp);
    mempool_destroy(mp);
    return 0;
}