#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>

#include  "shmring.h"
#include "shm_mempool.h"

#define RING_NAME "/shm-ring"
#define MP_NAME "/shm-mp"
#define HELLO_MSG "Hello, World!"

int main() {
    bool ret;
    shm_mp_handler_t mp_handler;
    shmring_handle_t shmring_handle;
    const int mp_size = 2048;

    const int bulk_size = 16;
    shm_mempool_node *mp_nodes[bulk_size];
    char *node_idxs[bulk_size];
    size_t ele_lens[bulk_size];

    size_t num_msg = 1008;

    /* section 1 */
    /* start shm mempool */
    mp_handler = shm_mp_init(256, mp_size, MP_NAME, strlen(MP_NAME));
    if (mp_handler == NULL) {
        perror("shm_mp_init failed.\n");
        return -1;
    }

    /* start bulk push shmring */
    shmring_handle = shmring_init(RING_NAME, strlen(RING_NAME));

    for (int i = 0; i < num_msg; i+=bulk_size)
    {
        for (int j = 0; j < bulk_size; j++)
        {
            mp_nodes[j] = shm_mp_malloc(mp_handler, 256);
            if (mp_nodes[j] == NULL) {
                perror("shm_mp_malloc failed. \n");
                return -1;
            }
            char *shm_mem = shm_offset_mem(mp_handler, mp_nodes[j]->node_idx);
            sprintf(shm_mem, "msg-%d: %s", (int)i+j, HELLO_MSG);
            printf("\t push node_idx=%d, msg='%s'\n", mp_nodes[j]->node_idx, shm_mem);

            node_idxs[j] = (char *)&mp_nodes[j]->node_idx;
            ele_lens[j] = sizeof(mp_nodes[j]->node_idx);
        }
        
        ret = shmring_push_bulk(shmring_handle, node_idxs, ele_lens, bulk_size);
        while(!ret) {
            fprintf(stderr, "shmring_push_bulk failed, retry...\n");
            sched_yield();
            ret = shmring_push_bulk(shmring_handle, node_idxs, ele_lens, bulk_size);
        }
        
    }

    printf("shmring_push_bulk pass \n");


    shmring_free(shmring_handle, 0);
    /* end shmring */

    shm_mp_destroy(mp_handler, 0);
    /* end section 1 */

    
    /* section 2  bulk pop shmring*/
    shm_mempool_node *pop_nodes[bulk_size];
    char * pop_node_idxs[bulk_size];
    int node_idx_values[bulk_size];
    size_t max_lens[bulk_size];
    int recv_cnt;

    for (int i = 0; i < bulk_size; i++)
    {
        max_lens[i] = sizeof(int);
        pop_node_idxs[i] = (char *) &node_idx_values[i];     
    }
    

    /* shm mempool */
    mp_handler = shm_mp_init(256, mp_size, MP_NAME, strlen(MP_NAME));
    if (mp_handler == NULL) {
        perror("shm_mp_init failed.\n");
        return -1;
    }

    /* end shm mempool */

    /* shmring */
    shmring_handle = get_shmring(RING_NAME, strlen(RING_NAME));

    for (int i = 0; i < num_msg; i+=bulk_size)
    {
        recv_cnt = shmring_pop_bulk(shmring_handle, pop_node_idxs, max_lens, bulk_size);
        while (recv_cnt == FAILED)
        {
            fprintf(stderr, "shmring_pop_bulk failed, retry... \n");
            sched_yield();
            recv_cnt = shmring_pop_bulk(shmring_handle, pop_node_idxs, max_lens, bulk_size);
        }

        if (recv_cnt != bulk_size) {
            fprintf(stderr, "partly pop %d\n", recv_cnt);
        }

        if (recv_cnt < bulk_size) {
            fprintf(stderr, "shmring_pop_bulk partly pop, retry... \n");
            recv_cnt = shmring_pop_bulk(shmring_handle, pop_node_idxs + recv_cnt, max_lens + recv_cnt, bulk_size - recv_cnt);
        }

        for (int j = 0; j < recv_cnt; j++)
        {
            int tmp_node_idx;
            tmp_node_idx = *(int *)pop_node_idxs[j];
            pop_nodes[j] = shm_mp_node(mp_handler, tmp_node_idx);
            char *tmp_shmmem = shm_offset_mem(mp_handler, tmp_node_idx);

            printf("\t pop node_idx=%d, msg='%s'\n", tmp_node_idx, tmp_shmmem);
            shm_mp_free(mp_handler, pop_nodes[j]);
        }
        printf("shm_mp_free pass\n");

    }
  
    shmring_free(shmring_handle, 1);
    /* end shmring */

    shm_mp_destroy(mp_handler, 1);
    /* end section 2 */

    return 0;
}
