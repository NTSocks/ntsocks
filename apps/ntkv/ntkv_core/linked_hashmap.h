#ifndef __LINKED_HASHMAP_H__
#define __LINKED_HASHMAP_H__
#include "mempool.h"

#define INIT_SIZE 4096
// #define CRITICAL_SIZE 1024
#define any_t void*
#define map_t void*
mempool_t *mp;

typedef int (*PUT)(map_t map, any_t key, any_t value);
typedef int (*GET)(map_t map, any_t key, any_t *value);
typedef int (*REMOVE)(map_t map, any_t key, void **key_addr, void **value_addr);
typedef void (*FREE)(map_t map);
typedef void (*DESTROY)(map_t map);
typedef int (*LEN)(map_t map);
typedef int (*HASHFUNC)(map_t map, any_t key);

// enum map_status{
//     MAP_OK,
//     MAP_REPLACE,
//     MAP_MISSING,
//     MAP_OMEM,
//     MAP_ERR
// };

typedef struct ele {
    any_t key;
    any_t value;
    struct ele *next;
}ele;

typedef struct ele* Entry;
typedef struct linked_hashmap{
    unsigned int size;
    unsigned int table_size;
    Entry entry;
    PUT put;
    GET get;
    REMOVE remove;
    FREE free;
    DESTROY destroy;
    LEN len;
    HASHFUNC hashcode;
} hashmap;

typedef struct Iterator{
    Entry entry;
    int iterator_count;
    int idx;
    int is_head;
    hashmap *map;
} Iterator;

map_t new_hashmap(unsigned int table_size, HASHFUNC hashcode);

int default_put(map_t map, any_t key, any_t value);
int default_get(map_t map, any_t key, any_t *value);
int default_remove(map_t map, any_t key, void **key_addr, void **value_addr);
void default_free(map_t map);
int default_len(map_t map);
int default_hashcode(map_t map, any_t key);
void default_destroy(map_t map);
void print_hashmap(map_t map);

void* key_addr(map_t map, any_t key);
void* value_addr(map_t map, any_t key);
Iterator* new_iterator(map_t map);
int has_next(Iterator *it);
Iterator* next(Iterator *it);
void free_iterator(Iterator *it);

#define PUT(map, key, value) ((hashmap*)map)->put(map, key, value)
#define GET(map, key, value) ((hashmap*)map)->get(map, key, value)
#define REMOVE(map, key, key_addr, value_addr) ((hashmap*)map)->remove(map, key, key_addr, value_addr)
#define FREE(map) ((hashmap*)map)->free(map)
#define DESTROY(map) ((hashmap*)map)->destroy(map)
#define LEN(map) ((hashmap*)map)->len(map)
#define HASHFUNC(map, key) ((hashmap*)map)->hashcode(map, key)

#endif