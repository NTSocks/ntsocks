#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>

#include "shmring.h"
#include "shm_mempool.h"

#define RING_NAME "/shm-ring"
#define MP_NAME "/shm-mp"
#define HELLO_MSG "Hello, World!"

int main(int argc, char *argv[])
{

        int ret;
        int node_idx;
        shm_mp_handler_t mp_handler;
        shmring_handle_t shmring_handle;
        int mp_size = 32;

        /* section 1 */
        /* start shm mempool */
        mp_handler = shm_mp_init(
            256, mp_size, MP_NAME, strlen(MP_NAME));
        if (mp_handler == NULL)
        {
                perror("shm_mp_init failed.\n");
                return -1;
        }

        shm_mempool_node *mp_node;
        mp_node = shm_mp_malloc(mp_handler, 256);
        if (mp_node == NULL)
        {
                perror("shm_mp_malloc failed. \n");
                return -1;
        }

        node_idx = mp_node->node_idx;
        char *shm_mem = shm_offset_mem(
            mp_handler, mp_node->node_idx);
        memcpy(shm_mem, HELLO_MSG, strlen(HELLO_MSG));

        shm_mempool_node *tmp_mp_node;
        tmp_mp_node = shm_mp_node_by_shmaddr(mp_handler, shm_mem);
        if (tmp_mp_node)
        {
                printf("[section 1] node_idx=%d, tmp_mp_node.node_idx=%d\n",
                       node_idx, tmp_mp_node->node_idx);
        }

        printf("[section 1] send msg payload: %s \n", shm_mem);
        /* end shm mempool */

        /* start shmring */
        shmring_handle =
            shmring_create((char *)RING_NAME,
                           strlen(RING_NAME), sizeof(node_idx), DEFAULT_MAX_BUFS);

        ret = shmring_push(shmring_handle,
                           (char *)(int *)&node_idx, sizeof(node_idx));
        while (ret)
        {
                sched_yield();
                ret = shmring_push(shmring_handle,
                                   (char *)(int *)&node_idx, sizeof(node_idx));
        }

        shmring_free(shmring_handle, false);
        /* end shmring */

        shm_mp_destroy(mp_handler, 0);
        /* end section 1 */

        /* section 2 */
        shm_mempool_node *tmp_node;
        char *tmp_shmmem;

        /* shm mempool */
        mp_handler = shm_mp_init(256,
                                 mp_size, MP_NAME, strlen(MP_NAME));
        if (mp_handler == NULL)
        {
                perror("shm_mp_init failed.\n");
                return -1;
        }

        // tmp_node = shm_mp_node(node_idx);
        // char * tmp_shmmem = shm_offset_mem(node_idx);
        /* end shm mempool */

        /* shmring */
        shmring_handle =
            shmring_init((char *)RING_NAME,
                         strlen(RING_NAME), sizeof(node_idx), DEFAULT_MAX_BUFS);

        int recv_idx;
        ret = shmring_pop(shmring_handle,
                          (char *)(int *)&recv_idx, sizeof(recv_idx));
        while (ret)
        {
                sched_yield();
                ret = shmring_pop(shmring_handle,
                                  (char *)(int *)&recv_idx, sizeof(recv_idx));
        }

        printf("\nidx=%d, recv_idx=%d \n\n", node_idx, recv_idx);

        tmp_node = shm_mp_node(mp_handler, recv_idx);
        tmp_shmmem = shm_offset_mem(mp_handler, recv_idx);
        printf("node_idx=%d, tmp_node.node_idx=%d, tmp_node.shm_offset=%ld, msg='%s'\n",
               node_idx, tmp_node->node_idx, tmp_node->shm_offset, tmp_shmmem);

        shm_mp_free(mp_handler, tmp_node);

        shmring_free(shmring_handle, true);
        /* end shmring */

        shm_mp_runtime_print(mp_handler);
        shm_mp_destroy(mp_handler, 1);
        /* end section 2 */

        return 0;
}