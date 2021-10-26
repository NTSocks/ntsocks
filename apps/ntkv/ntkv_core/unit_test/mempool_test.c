#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "../mempool.h"

#define EACH_CHUNK_SIZE 0.3 * GB
#define MAX_POOL_SIZE 2 * GB
#define NUM_DATA 40000
#define MAX_DATA_SIZE 80 * KB

static double total_used_size = 0;

typedef struct Item{
    int size;
    void *data;
}Item;
void test_fun(mempool_t *mempool);
int cmp(const void* item1, const void* item2){
    return ((struct Item*)item1)->size - ((struct Item*)item2)->size;  
}

int main(int argc, char* argv[]){
    srand((unsigned)time(NULL));
    clock_t start, end;
    double total_time;
    start = clock();
    DEBUG("INIT MEMPOOL");
    mempool_t *mempool = mempool_init(EACH_CHUNK_SIZE, MAX_POOL_SIZE, 0);

    test_fun(mempool);
    mempool_clear(mempool);
    mempool_destroy(mempool);

    end = clock();
    total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\n\tTotal time :%f seconds.\n", total_time);
    return 0;
}

void test_fun(mempool_t *mempool){
    struct Item data[NUM_DATA];
    int i, data_size;
    for (i = 0; i < NUM_DATA; ++i){
        data_size = rand() % MAX_DATA_SIZE;
        data_size = data_size > 0 ? data_size : KB;
        total_used_size += data_size;
        data[i].size = data_size;        
        data[i].data = mempool_alloc(mempool, data_size);
        memset(data[i].data, 0, data_size);
        DEBUG("mempool allocate:%d, allocate data size is %d, addr of data is %llu, total allocated size is %.4lf MB", i, data_size, data[i].data, total_used_size / MB);
        if (data[i].data == NULL){
            fprintf(stderr, "mempool allocate failed\n");
            return;
        }
    }
    printf("Before free:\n");
    print_mempool(mempool);
    
    for (i = 0; i < NUM_DATA / 2; ++i){
        mempool_free(mempool, data[i].data);
        data[i].data = NULL;
    }
    printf("After freeing half of memory:\n");
    print_mempool(mempool);
    for (i = 0; i < NUM_DATA / 2; ++i){
        data_size = rand() % MAX_DATA_SIZE;
        data_size = data_size > 0 ? data_size : KB;
        total_used_size += data_size;
        data[i].size = data_size;        
        data[i].data = mempool_alloc(mempool, data_size);
        memset(data[i].data, 0, data_size);
        DEBUG("mempool allocate:%d, allocate data size is %d, addr of data is %llu, total allocated size is %.4lf MB", i, data_size, data[i].data, total_used_size / MB);
        if (data[i].data == NULL){
            fprintf(stderr, "mempool allocate failed\n");
            return;
        }
    }
    printf("After reallocating half of memory:\n");
    print_mempool(mempool);

    qsort(data, NUM_DATA, sizeof(struct Item), cmp);
    DEBUG("FREE DATA");
    for (i = 0; i < NUM_DATA; ++i){
        DEBUG("free data:%d, data[%d].size=%d, data[%d].data=%llu", i, i, data[i].size, i, data[i].data);
        mempool_free(mempool, data[i].data);
        data[i].data = NULL;
    }
    printf("After free:\n");
    print_mempool(mempool);
    
    printf("Total used size: %lf\n", total_used_size);
}