#include <string.h>

#include "sem_shmring.h"

#define RING_NAME "sem_shmring"
#define MSG "Hello, World!"

int main()
{

    int retval;
    sem_shmring_handle_t handle;

    size_t addrlen = strlen(RING_NAME);
    size_t ele_size = 16;
    size_t ele_num = 8;

    handle = sem_shmring_create(
        (char *)RING_NAME, addrlen, ele_size, ele_num);
    if (!handle)
        return -1;

    int msg_cnt = 7;
    char send_msg[32];
    for (size_t i = 0; i < msg_cnt; i++)
    {
        sprintf(send_msg, "%s-%lu", MSG, i + 1);
        retval = sem_shmring_push(
            handle, send_msg, strlen(send_msg));
        if (retval)
        {
            return -1;
        }
        memset(send_msg, 0, 32);
    }

    sem_shmring_free(handle, 0);

    handle = sem_shmring_init(
        (char *)RING_NAME, addrlen, ele_size, ele_num);
    if (!handle)
        return -1;

    char recv_msg[32];
    for (size_t i = 0; i < msg_cnt; i++)
    {
        retval = sem_shmring_pop(handle, recv_msg, 32);
        if (retval)
            return -1;
        printf("recv msg %lu: %s \n", i + 1, recv_msg);
        memset(recv_msg, 0, 32);
    }

    sem_shmring_free(handle, 1);

    return 0;
}
