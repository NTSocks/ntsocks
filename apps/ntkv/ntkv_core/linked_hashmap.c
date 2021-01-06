#include "linked_hashmap.h"
#include "kv_errno.h"
#include "kv_log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int resize(map_t map, int new_size); 

// initialize hashmap
map_t new_hashmap(unsigned int table_size, HASHFUNC hashcode){
    // hashmap* map = (hashmap*)malloc(sizeof(hashmap));
    hashmap* map = (hashmap*) mempool_alloc(mp, sizeof(hashmap));
    memset(map, 0, sizeof(hashmap));
    if (!map) {
        fprintf(stderr, "malloc memory for hashmap failed\n");
        goto err;
    }
    
    if (table_size <= 0)
        table_size = INIT_SIZE;
    // map->entry = (Entry)calloc(table_size, sizeof(struct ele));
    map->entry = (Entry)mempool_alloc(mp, table_size * sizeof(struct ele));
    memset(map->entry, 0, sizeof(struct ele));
    if (!map->entry){
        fprintf(stderr, "calloc memory for hashmap entry failed\n");
        goto err;
    }
    DEBUG("addr of map is %llu, addr of entry is %llu", map, map->entry);

    map->table_size = table_size;
    map->size = 0;
    map->put = default_put;
    map->get = default_get;
    map->remove = default_remove;
    map->free = default_free;
    map->destroy = default_destroy;
    map->len = default_len;
    map->hashcode = hashcode == NULL ? default_hashcode : hashcode;
    // initialize hashmap each entry
    int i;
    for (i = 0; i < table_size; ++i)
        map->entry[i].key = map->entry[i].value = map->entry[i].next = NULL;
    return (map_t)map;
    err:
        if (map == NULL)
            return NULL;
        if (map->entry)
            mempool_free(mp, map->entry);
            // free(map->entry);
        mempool_free(mp, map);
        // free(map);
        return NULL;
}
// if the map exist the key, return MAP_REPLACE, else return MAP_OK.
int default_put(map_t map, any_t key, any_t value){
    assert(map);
    hashmap *m = (hashmap*) map;
    int idx = m->hashcode(map, key);
    // hashcode of the key does not exist
    if (m->entry[idx].key == NULL){
        DEBUG("key=%s, addr=%llu", key, key);
        m->entry[idx].key = key;
        m->entry[idx].value = value;
        m->size++;
    }else {
        // if the same key exists, replace the value directly
        Entry temp = &m->entry[idx];
        while (temp){
            if (strcmp((char*)key, (char*)temp->key) == 0){
                // DEBUG("[free] node data key addr=%llu, new key addr=%llu, new value addr=%llu, key=%s, newkey=%s, newvalue=%s", temp->key, key, value, temp->key, key, value);
                // mempool_free(mp, temp->key);
                // free(temp->key);
                //temp->key = key;
                temp->value = value;
                return MAP_REPLACE;
            }
            temp = temp->next;
        }
        // or add new entry
        // temp = (Entry)malloc(sizeof(struct ele));
        temp = (Entry)mempool_alloc(mp, sizeof(struct ele));
        //memset(temp, 0, sizeof(struct ele));
        temp->key = key;
        temp->value = value;
        temp->next = m->entry[idx].next;
        m->entry[idx].next = temp;
        m->size++;
    }
    // resize the hashmap
    if (m->size >= m->table_size){
        int new_size = 2 * m->table_size;
        // if (new_size <= CRITICAL_SIZE)
        //     new_size = 2 * new_size;
        // else
        //     new_size = new_size + CRITICAL_SIZE;
        DEBUG("[hashmap put resize]resize map, size=%d, old_size=%d, new_size=%d", m->size, m->table_size, new_size);
        int status = resize(map, new_size);
        if (status != MAP_OK)
            return status;
    }
    return MAP_OK;
}
// if the map exist the key, return MAP_OK, else return MAP_MISSING.
int default_get(map_t map, any_t key, any_t *value){
    assert(map);
    hashmap *m = (hashmap*) map;
    int idx = m->hashcode(map, key);
    if (m->entry[idx].key != NULL){
        Entry curr = &m->entry[idx];
        while (curr){
            if (strcmp((char*)curr->key, (char*)key) == 0){
                *value = curr->value;
                return MAP_OK;
            }
            curr = curr->next;
        }
    }
    return MAP_MISSING;
}
// if the map exist the key, return MAP_OK, else return MAP_MISSING.
int default_remove(map_t map, any_t key, void **key_addr, void **value_addr){
    assert(map);
    hashmap *m = (hashmap*) map;
    int idx = m->hashcode(map, key);
    if (m->entry[idx].key == NULL){
        return MAP_MISSING;
    }
    Entry head = &m->entry[idx];
    if (strcmp((char*)head->key, (char*)key) == 0){
        DEBUG("[free] head key addr, key addr=%llu, value addr=%llu, key=%s, value=%s", head->key, head->value, head->key, head->value);
        // free(head->key);
        *key_addr = head->key;
        if (head->next == NULL){
            head->key = NULL;
            head->value = NULL;
        }else {
            Entry remove = head->next;
            head->key = remove->key;
            head->value = remove->value;
            head->next = remove->next;
            remove->key = NULL;
            remove->value = NULL;
            remove->next = NULL;
            DEBUG("[free] node addr, node addr=%llu", remove);
            mempool_free(mp, remove);
            // free(remove);
        }
        m->size--;
        goto ok;
    }else {
        Entry next = head->next;
        while (next){
            if (strcmp((char*)next->key, (char*)key) == 0){
                head->next = next->next;
                DEBUG("[free] node key addr, key addr=%llu", next->key);
                // free(next->key);
                *key_addr = next->key;
                // next->key = NULL;
                // next->value = NULL;
                // next->next = NULL;
                // DEBUG("[free] node addr, node addr=%llu", next);
                // free(next);
                mempool_free(mp, next);
                m->size--;
                goto ok;
            }
            head = next;
            next = next->next;
        }
    }
    return MAP_MISSING;
    ok:
    if (m->table_size >INIT_SIZE && m->size < (m->table_size / 2)){
        DEBUG("[hashmap remove resize]resize map, size=%d, old_size=%d, new_size=%d", m->size, m->table_size, m->table_size / 2);
        int status = resize(map, m->table_size / 2);
        if (status != MAP_OK)
            return status;
    }
    return MAP_OK;
}
// free the map
void default_free(map_t map){
    assert(map);
    hashmap *m = (hashmap*) map;
    if (m){
        if(m->entry){
            int i;
            for (i = 0; i < m->table_size; i++) {
                Entry curr = m->entry[i].next;
                DEBUG("addr=%llu, key=%s, value=%s", m->entry[i].key, m->entry[i].key, m->entry[i].value);
                while (curr){
                    Entry next = curr->next;
                    curr->key = NULL;
                    curr->value = NULL;
                    curr->next = NULL;
                    DEBUG("addr=%llu, key=%s, value=%s", curr->key, curr->key, curr->value);
                    // DEBUG("[free] node addr, addr=%llu", curr);
                    mempool_free(mp, curr);
                    // free(curr);
                    curr = next;
                }
                m->entry[i].next = NULL;
            }
        }
        m->size = 0;
    }
}
// return the number of elements
int default_len(map_t map){
    assert(map);
    hashmap *m = (hashmap*) map;
    if (m)
        return m->size;
    return 0;
}
// resize the map
static int resize(map_t map, int new_size){
    assert(map);
    // print_hashmap(map);
    hashmap *m = (hashmap*) map;
    int old_table_size = m->table_size;
    int old_size = m->size;
    m->table_size = new_size;
    m->size = 0;
    Entry old_entry = m->entry;
    // get and remove all key-value pairs
    // Entry entrys = (Entry)calloc(old_size, sizeof(struct ele));
    Entry entrys = (Entry)mempool_alloc(mp, old_size * sizeof(struct ele));
    memset(entrys, 0, old_size * sizeof(struct ele));
    int i, k = 0;
    for (i = 0; i < old_size; ++i){
        Entry curr = &old_entry[i];
        if(curr->key != NULL){
            entrys[k].key = curr->key;
            entrys[k].value = curr->value;
            entrys[k++].next = NULL;
            while (curr->next){
                entrys[k].key = curr->next->key;
                entrys[k].value = curr->next->value;
                entrys[k++].next = NULL;
                Entry temp = curr->next;
                curr->next = temp->next;
                // temp->key = NULL;
                // temp->value = NULL;
                // temp->next = NULL;
                DEBUG("[free] old node addr, addr=%llu", temp);
                mempool_free(mp, temp);
                // free(temp);
            }
        }
    }
    mempool_free(mp, old_entry);
    // free(old_entry);
    // m->entry = (Entry)calloc(m->table_size, sizeof(struct ele));
    m->entry = (Entry)mempool_alloc(mp, m->table_size * sizeof(struct ele));
    memset(m->entry, 0, m->table_size * sizeof(struct ele));
    if (m->entry == NULL){
        fprintf(stderr, "calloc for hashmap entry failed\n");
        return MAP_OMEM;
    }
    for (i = 0; i < old_size; ++i){
        default_put((map_t)m, entrys[i].key, entrys[i].value);
    }
    mempool_free(mp, entrys);
    // free(entrys);
    // print_hashmap(map);
    return MAP_OK;
}
// print map info
void print_hashmap(map_t map){
    assert(map);
    hashmap *m = (hashmap*) map;
    if (m == NULL)
        return;
    int i;
    for (i = 0; i < m->table_size; ++i){
        Entry curr = &m->entry[i];
        printf("entry[%d]:",i);
        if (curr != NULL && curr->key != NULL){ 
            while (curr != NULL && curr->key != NULL){
                printf("[%llu, %s, %s]->", curr->key, curr->key, curr->value);
                curr = curr->next;
            }  
        }
        printf("\n");
    }
}

void default_destroy(map_t map){
    assert(map);
    hashmap *m = (hashmap*) map;
    if (m == NULL)
        return;
    DEBUG("addr of map is %llu, addr of entry is %llu", m, m->entry);
    if (m->entry)
        mempool_free(mp, m->entry);
        // free(m->entry);
    mempool_free(mp, m);
    // free(m);
}

// get key address
void* key_addr(map_t map, any_t key){
    assert(map);
    hashmap *m = (hashmap*) map;
    int idx = m->hashcode(map, key);
    if (m->entry[idx].key != NULL){
        Entry curr = &m->entry[idx];
        while (curr){
            if (strcmp((char*)curr->key, (char*)key) == 0){
                return curr->key;
            }
            curr = curr->next;
        }
    }
    return NULL;
}

// get value address
void* value_addr(map_t map, any_t key){
    assert(map);
    hashmap *m = (hashmap*) map;
    int idx = m->hashcode(map, key);
    if (m->entry[idx].key != NULL){
        Entry curr = &m->entry[idx];
        while (curr){
            if (strcmp((char*)curr->key, (char*)key) == 0){
                return curr->value;
            }
            curr = curr->next;
        }
    }
    return NULL;
}

Iterator* new_iterator(map_t map){
    assert(map);
    // Iterator *it = (Iterator*)malloc(sizeof(Iterator));
    Iterator *it = (Iterator*)mempool_alloc(mp, sizeof(Iterator));
    memset(it, 0, sizeof(Iterator));
    it->map = (hashmap*) map;
    it->iterator_count = 0;
    it->idx = 0;
    it->is_head = 0;
    it->entry = NULL;
    return it;
}

int has_next(Iterator *it){
    return it->iterator_count < it->map->size ? 1 : 0;
}

Iterator* next(Iterator *it){
    if (has_next(it)){
        if (it->entry != NULL && it->entry->next != NULL){
            it->iterator_count++;
            it->entry = it->entry->next;
            it->is_head = 0;
            return it;
        }
        while (it->idx < it->map->table_size){
            Entry head = &it->map->entry[it->idx];
            it->idx++;
            if (head->key != NULL){
                it->iterator_count++;
                it->entry = head;
                it->is_head = 1;
                return it;
            }
        }
    }
    return NULL;
}

void free_iterator(Iterator *it){
    it->entry = NULL;
    it->map = NULL;
    mempool_free(mp, it);
    // free(it);
}

// hash function
int default_hashcode(map_t map, any_t key) {
    char * k = (char *)key;
    unsigned long h = 0;
    while (*k) {
        h = (h << 4) + *k++;
        unsigned long g = h & 0xF0000000L;
        if (g) {
            h ^= g >> 24;
        }
        h &= ~g;
    }
    return h % ((hashmap*)map)->table_size;
}