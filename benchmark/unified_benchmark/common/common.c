#include "common.h"

log_ctx_t log_init(char *filepath, size_t pathlen)
{
    if (filepath == NULL || pathlen <= 0)
    {
        return NULL;
    }

    struct log_context *ctx;
    ctx = (log_ctx_t)malloc(sizeof(struct log_context));
    if (!ctx)
    {
        return NULL;
    }

    memset(ctx->filepath, 0, 128);
    memcpy(ctx->filepath, filepath, pathlen);
    ctx->file = fopen(ctx->filepath, "a+");
    if (!ctx->file)
    {
        free(ctx);
        return NULL;
    }

    return ctx;
}

void log_append(log_ctx_t ctx, char *msg, size_t msglen)
{
    if (!ctx || !msg || msglen <= 0)
        return;

    fwrite(msg, msglen, 1, ctx->file);
}

void log_destroy(log_ctx_t ctx)
{
    if (!ctx || !ctx->file)
        return;

    fclose(ctx->file);
}

// each thread pins to one core
void pin_1thread_to_1core(int core_id)
{
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();
    CPU_ZERO(&cpuset);

    int s;
    CPU_SET(core_id, &cpuset);

    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
        fprintf(stderr, "pthread_setaffinity_np:%d", s);
    }

    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
        fprintf(stderr, "pthread_getaffinity_np:%d", s);
    }

}

int get_core_id(int *last_core, int cpucores, int inc_or_dec)
{
    if (inc_or_dec == 0)
    {
        *last_core = (*last_core + 2) % cpucores;
    }
    else
    {
        *last_core = *last_core - 2;
        if (*last_core < 0)
        {
            *last_core += cpucores;
        }
    }
    return *last_core;
}

/**
 *   get core from 16-31,48-63.
 *   when inc_or_dec is 0, allocate from small to large.
 *   when inc_or_dec is 1, allocate from large to small.
 */
int get_core_id2(int *last_core, int inc_or_dec)
{
    if (inc_or_dec == 0)
    {
        *last_core = *last_core + 1;
        // NUMA node0 CPU(s):   0-15,32-47
        // NUMA node1 CPU(s):   16-31,48-63

        if (*last_core == 32)
        {
            *last_core = 48;
        }
        else if (*last_core == 64)
        {
            *last_core = 16;
        }
    }
    else if (inc_or_dec == 1)
    {
        *last_core = *last_core - 1;
        // NUMA node0 CPU(s):   0-15,32-47
        // NUMA node1 CPU(s):   16-31,48-63
        if (*last_core == 15)
        {
            *last_core = 63;
        }
        else if (*last_core == 47)
        {
            *last_core = 31;
        }
    }
    return *last_core;
}
/*
    get core from 0-15,32-47
    when inc_or_dec is 0, allocate from small to large.
    when inc_or_dec is 1, allocate from large to small.
*/
int get_core_id3(int *last_core, int inc_or_dec)
{
    if (inc_or_dec == 0)
    {
        *last_core = *last_core + 1;
        // NUMA node0 CPU(s):   0-15,32-47
        // NUMA node1 CPU(s):   16-31,48-63
        if (*last_core == 16)
        {
            *last_core = 32;
        }
        else if (*last_core == 48)
        {
            *last_core = 0;
        }
    }
    else if (inc_or_dec == 1)
    {
        *last_core = *last_core - 1;
        // NUMA node0 CPU(s):   0-15,32-47
        // NUMA node1 CPU(s):   16-31,48-63
        if (*last_core == 31)
        {
            *last_core = 15;
        }
        else if (*last_core == -1)
        {
            *last_core = 47;
        }
    }
    return *last_core;
}