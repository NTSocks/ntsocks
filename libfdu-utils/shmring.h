#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

// max bytes of total bufs: 8 MB, = 256 * 32768 [(32 * 1024) = 32768]
#define MAX_BUFS 1024 // define NTP_MAX_BUFS as [(a power of 2) -1] (65535 in our case)
#define BUF_SIZE sizeof(int)

#define NTP_SHM_NAME "/nts-shm-ring"

#define NTS_LIKELY(x) __builtin_expect(!!(x), 1)
#define NTS_UNLIKELY(x) __builtin_expect(!!(x), 0)

typedef struct _shmring shmring_t;
typedef shmring_t* shmring_handle_t;


shmring_handle_t shmring_init(char *shm_addr, size_t addrlen);

shmring_handle_t get_shmring(char *shm_addr, size_t addrlen);

bool shmring_push(shmring_handle_t self, char *element, size_t ele_len);

bool shmring_pop(shmring_handle_t self, char *element, size_t ele_len);

bool shmring_push_bulk(shmring_handle_t self, char **elements, size_t *ele_lens, size_t count);

size_t shmring_pop_bulk(shmring_handle_t self, char **elements, size_t *max_lens, size_t count);

bool shmring_front(shmring_handle_t self, char *element, size_t ele_len);

void shmring_free(shmring_handle_t self, int unlink);

uint64_t ntp_get_read_index(shmring_handle_t self);

uint64_t ntp_get_peer_read_index(shmring_handle_t self);

int ntp_set_peer_read_index(shmring_handle_t self, uint64_t read_index);



#ifdef __cplusplus
};
#endif