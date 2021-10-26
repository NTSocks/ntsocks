#ifndef SHM_RING_BUFFER_UTILS_H
#define SHM_RING_BUFFER_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <uuid/uuid.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define UUID_LEN 36

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

//MEMORY read & write barrier
#define __MEM_BARRIER                \
    __asm__ __volatile__("mfence" :: \
                             : "memory")
//memory read barrier
#define __READ_BARRIER__             \
    __asm__ __volatile__("lfence" :: \
                             : "memory")
//memory write barrier
#define __WRITE_BARRIER__            \
    __asm__ __volatile__("sfence" :: \
                             : "memory")

#define SUCCESS 0
#define FAILED -1

#define NEW(type) (type *)malloc(sizeof(type))
#define APPLY(task, ...) task(__VA_ARGS__)
#define ABS(n) n > 0 ? n : -n
#define RANGE(min, max, number) number<min ? min : number> max ? max : number
#define EQUAL(a, b) a == b ? 0 : a < b ? -1 \
                                       : 1
#define log printf

    // uuid
    char *createUUID();

    char *generate_uuid();

    int parse_sockaddr_port(struct sockaddr_in *saddr);

#ifdef __cplusplus
};
#endif

#endif //SHM_RING_BUFFER_UTILS_H
