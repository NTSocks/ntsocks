/*
 * <p>Title: nt_spsc_shm_queue.h</p>
 * <p>Description: Single-Producer and Single-Consumer lock-free ring buffer </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Jan 6, 2020 
 * @version 1.0
 */

#ifndef NT_SPSC_SHM_QUEUE_H_
#define NT_SPSC_SHM_QUEUE_H_

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _nt_spsc_shmring nt_spsc_shmring_t;
    typedef nt_spsc_shmring_t *nt_spsc_shmring_handle_t;

    /**
     *
     * @param shm_addr
     * @param addrlen
     * @param element_num : the power of 2
     * @param element_size
     * @return
     */
    nt_spsc_shmring_handle_t
    nt_spsc_shmring_init(
        char *shm_addr, size_t addrlen, int element_num, size_t element_size);

    /**
     *
     * @param shm_addr
     * @param addrlen
     * @param element_num : the power of 2
     * @param element_size
     * @return
     */
    nt_spsc_shmring_handle_t
    nt_get_spsc_shmring(
        char *shm_addr, size_t addrlen, int element_num, size_t element_size);

    bool nt_spsc_shmring_push(
        nt_spsc_shmring_handle_t self, char *element, size_t element_size);

    bool nt_spsc_shmring_pop(
        nt_spsc_shmring_handle_t self, char *element, size_t element_size);

    void nt_spsc_shmring_free(nt_spsc_shmring_handle_t self, int unlink);

    bool nt_spsc_shmring_is_full(nt_spsc_shmring_handle_t self);

#ifdef __cplusplus
};
#endif

#endif /* NT_SPSC_SHM_QUEUE_H_ */
