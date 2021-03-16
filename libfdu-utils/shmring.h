#ifndef __SHMRING__H__
#define __SHMRING__H__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// max bytes of total bufs: 8 MB, = 256 * 32768 [(32 * 1024) = 32768]
// define NTP_MAX_BUFS as [(a power of 2) -1] (65535 in our case)
#define DEFAULT_MAX_BUFS 1024
#define NTP_SHM_NAME "nts-shm-ring"

        typedef struct _shmring shmring_t;
        typedef shmring_t *shmring_handle_t;

        typedef enum _shm_stat
        {
                SHM_UNREADY = 0,
                SHM_READY,
                SHM_CLOSE,
                SHM_UNLINK
        } shm_stat;

        shmring_handle_t
        shmring_create(char *shm_addr,
                       size_t addrlen, size_t ele_size, size_t ele_num);

        shmring_handle_t
        shmring_init(char *shm_addr,
                     size_t addrlen, size_t ele_size, size_t ele_num);

        int shmring_push(
            shmring_handle_t self, char *element, size_t ele_len);

        int shmring_pop(
            shmring_handle_t self, char *element, size_t ele_len);

        int shmring_push_bulk(
            shmring_handle_t self, char **elements,
            size_t *ele_lens, size_t count);

        size_t shmring_pop_bulk(shmring_handle_t self,
                                char **elements, size_t *max_lens, size_t count);

        int shmring_front(shmring_handle_t self,
                           char *element, size_t ele_len);

        void shmring_free(shmring_handle_t self, bool is_unlink);

        uint64_t ntp_get_read_index(shmring_handle_t self);

        uint64_t ntp_get_peer_read_index(shmring_handle_t self);

        int ntp_set_peer_read_index(
            shmring_handle_t self, uint64_t read_index);

#ifdef __cplusplus
};
#endif
#endif //!__SHMRING__H__
