
#include <stdio.h>
#include <stdlib.h>

#include "ntp2nts_shm.h"

#define NTP_SHM "/ntp-shm"
#define MSG "Hello World!"


int main() {

    ntp_shm_context_t ntp_shm_ctx;
    ntp_shm_ctx = ntp_shm();

    int retval;
    retval = ntp_shm_accept(ntp_shm_ctx, NTP_SHM, strlen(NTP_SHM));
    if(retval == -1) {
        perror("ntp_shm_accept failed \n");
        return -1;
    }

    for (size_t i = 0; i < 13; i++)
    {
        shm_mempool_node *mp_node;
        mp_node = shm_mp_malloc(ntp_shm_ctx->mp_handler, sizeof(ntp_msg));
        if(mp_node) {
            shm_mp_free(ntp_shm_ctx->mp_handler, mp_node);
        }
    }
    


    for (size_t i = 0; i < 3; i++)
    {
        shm_mempool_node *mp_node;
        mp_node = shm_mp_malloc(ntp_shm_ctx->mp_handler, sizeof(ntp_msg));
        if (mp_node == NULL) {
            perror("shm_mp_malloc failed. \n");
            return -1;
        }

        ntp_msg * msg = (ntp_msg *) shm_offset_mem(ntp_shm_ctx->mp_handler, mp_node->node_idx);
        if (msg == NULL) {
            perror("shm_offset_mem failed \n");
            return -1;
        }

        msg->msg_type = NTP_NTS_MSG_DATA;
        sprintf(msg->msg, "msg-%d", (int)i);
        msg->msg_len = strlen(msg->msg);


        retval = ntp_shm_send(ntp_shm_ctx, msg);
        if(retval == -1) {
            perror("ntp_shm_send failed \n");
            return -1;
        }

        printf("send '%s' success\n", msg->msg);

    }
    

    ntp_shm_nts_close(ntp_shm_ctx);
    ntp_shm_destroy(ntp_shm_ctx);

    
    ntp_shm_ctx = ntp_shm();
    retval = ntp_shm_connect(ntp_shm_ctx, NTP_SHM, strlen(NTP_SHM));
    if(retval == -1) {
        perror("ntp_shm_connect failed\n");
        return -1;
    }

    for (size_t i = 0; i < 3; i++)
    {
        ntp_msg * recv_msg;
        recv_msg = ntp_shm_recv(ntp_shm_ctx);
        if(recv_msg == NULL) {
            perror("ntp_shm_recv failed \n");
            return -1;
        }

        printf("msg_len=%d, recv msg: '%s' \n", recv_msg->msg_len, recv_msg->msg);

        shm_mempool_node * tmp_node;
        tmp_node = shm_mp_node_by_shmaddr(ntp_shm_ctx->mp_handler, (char *)recv_msg);
        if(tmp_node) {
            shm_mp_free(ntp_shm_ctx->mp_handler, tmp_node);
        }
    }
    

    // shm_mp_free(ntp_shm_ctx->mp_handler, mp_node);


    ntp_shm_close(ntp_shm_ctx);
    ntp_shm_destroy(ntp_shm_ctx);


    return 0;
}


