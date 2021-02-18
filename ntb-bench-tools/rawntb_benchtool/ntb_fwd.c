#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>

#include <rte_io.h>
#include <rte_eal.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_rawdev.h>
#include <rte_rawdev_pmd.h>
#include <rte_memcpy.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_debug.h>
#include <rte_dev.h>
#include <rte_kvargs.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_lcore.h>
#include <rte_bus.h>
#include <rte_bus_vdev.h>
#include <rte_memzone.h>
#include <rte_mempool.h>
#include <rte_rwlock.h>
#include <rte_ring.h>
#include <rte_mbuf.h>
#include <rte_cycles.h>

#include "nt_log.h"
#include "ntb.h"
#include "ntb_hw_intel.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

#define RUN_TIME 5
#define CACHE_LINE_SIZE 64
//内存屏障
#define __MEM_BARRIER                \
    __asm__ __volatile__("mfence" :: \
                             : "memory")
//内存读屏障
#define __READ_BARRIER__             \
    __asm__ __volatile__("lfence" :: \
                             : "memory")
//内存写屏障
#define __WRITE_BARRIER__            \
    __asm__ __volatile__("sfence" :: \
                             : "memory")

enum METRIC
{
    RW_TPUT = 1,
    RW_LAT,
    RR_TPUT,
    RR_LAT
};

int payload_size = 16;
size_t num_req = 10000;
// 1 - tput(remote write), 2 - lat(remote write), 3 - bw(remote write)
// 4 - tput(remote read), 5 - lat(remote read), 6 - bw(remote read)
int metric = 1;
int thrds = 1;
bool start = false;
bool stop = false;
double **result_lat;
double *result_tput;

struct thrds_argv
{
    struct ntb_link *link;
    int sublink_idx;
};

struct ntb_sublink
{
    volatile uint8_t *sub_local_ptr;
    volatile uint8_t *sub_remote_ptr;
};

struct ntb_link
{
    struct rte_rawdev *dev;
    struct ntb_hw *hw;
    // 指向本地NTB的基地址
    volatile uint8_t *local_ptr;
    // 指向对端NTB PCIe设备的基地址
    volatile uint8_t *remote_ptr;

    volatile uint8_t *cur_local_ptr;
    volatile uint8_t *cur_remote_ptr;

    struct ntb_sublink **sublink;
};

struct ntb_link *raw_ntb_start(uint16_t dev_id);
bool check_ntb_link_status(uint16_t dev_id);
int find_raw_ntb_device(void);
static int lcore_ntb_daemon(void);
void parse_args(int argc, char *argv[]);
void usage(void);
void latency_report_perf(size_t *start_tsc, size_t *end_tsc, int thrd_id);
int cmp(const void *a, const void *b);
void ntb_connect(struct ntb_link *link);
void *ntb_tput_rw(void *argv);
void *ntb_tput_rr(void *argv);
void *ntb_lat_rw(void *argv);
void *ntb_lat_rr(void *argv);
void *ntb_bw_rw(void *argv);
void *ntb_bw_rr(void *argv);
void pin_1thread_to_1core(int thread_id);

static void report_result_tput(void);
static void report_result_lat(void);
static void ntb_close(struct ntb_link *link, uint16_t dev_id);

// each thread pins to one core
void pin_1thread_to_1core(int thread_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id, &cpuset);
    int s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
        fprintf(stderr, "pthread_setaffinity_np:%d", s);
    s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
        fprintf(stderr, "pthread_getaffinity_np:%d", s);
}

#pragma GCC diagnostic ignored "-Wcast-qual"
int cmp(const void *a, const void *b)
{
    return *(double *)a > *(double *)b ? 1 : -1;
}

void ntb_connect(struct ntb_link *link)
{
    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        DEBUG("I am NTB_TOPO_B2B_USD");
        const char *str = "start";
        DEBUG("send `start` msg");
        rte_memcpy((void *)link->remote_ptr, str, strlen(str) + 1);

        DEBUG("ready to recv `ack` msg");
        const char *ack = (char *)link->local_ptr;
        while (strcmp(ack, "ack") != 0)
            sched_yield();
        DEBUG("recv `ack` msg done.");
        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        DEBUG("I am NTB_TOPO_B2B_DSD");
        char *str = (char *)link->local_ptr;
        DEBUG("ready to recv `start` msg");
        while (strcmp(str, "start") != 0)
            sched_yield();
        DEBUG("recv `start` msg done.");

        const char *ack = "ack";
        DEBUG("ready to send `ack` msg");
        rte_memcpy((void *)link->remote_ptr, ack, strlen(ack) + 1);
        DEBUG("send `ack` msg");
        break;
    }

    default:
        break;
    }
}

void *ntb_tput_rw(void *argv)
{
    struct thrds_argv *targv = ((struct thrds_argv *)argv);
    struct ntb_link *link = targv->link;
    struct ntb_sublink *sublink = link->sublink[targv->sublink_idx];
    pin_1thread_to_1core(targv->sublink_idx);

    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        while (!start)
            sched_yield();

        // uint64_t prev_tsc, cur_tsc, timer_tsc = 0, old_timer_tsc = 0;
        // int run_times = 1;
        char start = 's', end = 'e';

        char str[payload_size];
        // memset((void *)str, 'a', payload_size);
        str[0] = str[payload_size - 1] = start;

        // memset((void *)msg, 'a', payload_size);
        sublink->sub_local_ptr[0] = 'a';

        // uint64_t counter = 0, old_counter = 0;
        uint64_t counter = 0;
        // double elapse_time = 0.0;

        while (1)
        {
            // prev_tsc = rte_rdtsc();

            rte_memcpy((void *)sublink->sub_remote_ptr, str, payload_size);

            while (sublink->sub_local_ptr[0] != end)
                sched_yield();
            sublink->sub_local_ptr[0] = start;

            // cur_tsc = rte_rdtsc();

            counter++;

            if (stop)
                goto STOP_TPUT_RW;
            // timer_tsc += (cur_tsc - prev_tsc);
            // if (unlikely(timer_tsc >= run_times * rte_get_timer_hz())){
            //     run_times++;

            //     elapse_time = (double)(timer_tsc - old_timer_tsc) / rte_get_timer_hz();
            //     printf("[thread-%d remote write] elapse_time = %.2lf, loop counter = %ld, TPUT (req/s) = %.2lf \n", targv->sublink_idx, elapse_time, counter - old_counter, (double)(counter - old_counter) / elapse_time);

            //     old_timer_tsc = timer_tsc;
            //     old_counter = counter;

            //     if (run_times > RUN_TIME){
            //         break;
            //     }
            // }
        }

    // elapse_time = (double)timer_tsc / rte_get_timer_hz();
    // printf("[thread-%d remote write] send buff = %d, total elapse_time = %.2lf, loop counter = %ld, TPUT (req/s) = %.2lf \n", targv->sublink_idx, payload_size, elapse_time, counter, (double)counter / elapse_time);
    STOP_TPUT_RW:
        printf("[thread-%d remote write] send buff = %d, total elapse_time = %d, loop counter = %ld, TPUT (req/s) = %.2lf \n", targv->sublink_idx, payload_size, RUN_TIME, counter, (double)counter / RUN_TIME);
        result_tput[targv->sublink_idx] = (double)counter / RUN_TIME;
        sublink->sub_remote_ptr[1] = end;
        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        char start = 's', end = 'e';
        // memset((void *)str, 'a', payload_size);
        // str[0] = str[payload_size - 1] = 'a';

        char ack[1];
        // memset((void *)ack, 'a', payload_size);
        ack[0] = end;

        while (1)
        {
            // recv msg
            while (sublink->sub_local_ptr[0] != start || sublink->sub_local_ptr[payload_size - 1] != start)
                if (sublink->sub_local_ptr[1] != end)
                    sched_yield();
                else
                    goto EXIT;
            sublink->sub_local_ptr[0] = sublink->sub_local_ptr[payload_size - 1] = end;

            rte_memcpy((void *)sublink->sub_remote_ptr, ack, 1);
        }

    EXIT:
        break;
    }

    default:
        break;
    }

    return NULL;
}

void *ntb_tput_rr(void *argv)
{
    struct thrds_argv *targv = ((struct thrds_argv *)argv);
    struct ntb_link *link = targv->link;
    struct ntb_sublink *sublink = link->sublink[targv->sublink_idx];
    pin_1thread_to_1core(targv->sublink_idx);

    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        while (!start)
            sched_yield();

        // uint64_t prev_tsc, cur_tsc, timer_tsc = 0, old_timer_tsc = 0;
        // int run_times = 1;

        char start = 's', end = 'e';
        char str[payload_size];
        // memset((void *)str, 'a', payload_size);
        // 如果定义为int，有可能会溢出
        // uint64_t counter = 0, old_counter = 0;
        uint64_t counter = 0;
        // double elapse_time = 0.0;

        // 忽略第一个msg
        rte_memcpy((void *)str, (void *)sublink->sub_remote_ptr, payload_size);
        while (str[0] != start || str[payload_size - 1] != start)
            sched_yield();
        // reset msg
        str[0] = str[payload_size - 1] = end;

        while (1)
        {
            // prev_tsc = rte_rdtsc();

            rte_memcpy((void *)str, (void *)sublink->sub_remote_ptr, payload_size);
            while (str[0] != start || str[payload_size - 1] != start)
                sched_yield();
            // reset msg
            str[0] = str[payload_size - 1] = end;

            // cur_tsc = rte_rdtsc();

            counter++;

            if (stop)
                goto STOP_TPUT_RR;
            // timer_tsc += (cur_tsc - prev_tsc);

            // if (unlikely(timer_tsc >= run_times * rte_get_timer_hz())){
            //     run_times++;

            //     elapse_time = (double)(timer_tsc - old_timer_tsc) / rte_get_timer_hz();
            //     printf("[thread-%d remote read] elapse_time = %.2lf, loop counter = %ld, TPUT (req/s) = %.2lf \n", targv->sublink_idx, elapse_time, counter - old_counter, (double)(counter - old_counter) / elapse_time);

            //     old_timer_tsc = timer_tsc;
            //     old_counter = counter;

            //     if (run_times > RUN_TIME)
            //         break;
            // }
        }

    // elapse_time = (double)timer_tsc / rte_get_timer_hz();
    // printf("[thread-%d remote read] send buff = %d, total elapse_time = %.2lf, loop counter = %ld, TPUT (req/s) = %.2lf \n", targv->sublink_idx, payload_size, elapse_time, counter, (double)counter / elapse_time);
    STOP_TPUT_RR:
        printf("[thread-%d remote read] send buff = %d, total elapse_time = %d, loop counter = %ld, TPUT (req/s) = %.2lf \n", targv->sublink_idx, payload_size, RUN_TIME, counter, (double)counter / RUN_TIME);
        result_tput[targv->sublink_idx] = (double)counter / RUN_TIME;
        sublink->sub_local_ptr[0] = end;
        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        char *lp = (char *)sublink->sub_local_ptr;
        char *rp = (char *)sublink->sub_remote_ptr;
        // memset((void *)lp, 'a', payload_size);
        // memset((void *)rp, 'a' ,payload_size);

        char start = 's', end = 'e';
        lp[0] = lp[payload_size - 1] = start;

        while (rp[0] != end)
            sched_yield();

        break;
    }

    default:
        break;
    }

    return NULL;
}

void *ntb_lat_rw(void *argv)
{
    struct thrds_argv *targv = ((struct thrds_argv *)argv);
    struct ntb_link *link = targv->link;
    struct ntb_sublink *sublink = link->sublink[targv->sublink_idx];
    pin_1thread_to_1core(targv->sublink_idx);

    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        while (!start)
            sched_yield();
        // printf("I am NTB_TOPO_B2B_USD measuring ntb latency\n");
        size_t *start_tsc, *end_tsc;
        start_tsc = (size_t *)malloc(sizeof(size_t) * num_req);
        end_tsc = (size_t *)malloc(sizeof(size_t) * num_req);
        char str[payload_size];
        memset((void *)str, 'a', payload_size);

        char start = 's', end = 'e';
        str[0] = str[payload_size - 1] = start;

        volatile char *msg = (volatile char *)sublink->sub_local_ptr;
        memset((void *)msg, 'a', payload_size);
        //sprintf(msg, "start\0");
        volatile char *rp = (volatile char *)sublink->sub_remote_ptr;

        // char ss[4*num_req];
        for (size_t i = 0; i < num_req; ++i)
        {
            // send msg
            start_tsc[i] = rte_rdtsc();
            // rte_io_rmb();
            rte_memcpy((void *)rp, str, payload_size);
            // printf("1111---%u, %c-%c\n", i, msg[0], msg[payload_size - 1]);
            // rte_io_rmb();
            while (msg[0] != end || msg[payload_size - 1] != end)
                sched_yield();
            // ss[4*i+2] = msg[0];
            // ss[4*i+3] = msg[payload_size - 1];
            // while (msg[payload_size - 1] != end) sched_yield();
            // printf("4444---%u, %c-%c\n", i, msg[0], msg[payload_size - 1]);
            // rte_io_rmb();
            msg[0] = start;
            msg[payload_size - 1] = start;
            // rte_io_rmb();
            // ss[4*i] = msg[0];
            // ss[4*i+1] = msg[payload_size - 1];
            // printf("2222---%u, %c-%c\n", i, msg[0], msg[payload_size - 1]);
            // while (msg[0] != start || msg[payload_size - 1] != start) {
            //     printf("xxx---%u, %c-%c-%c-%c-%c-%c\n", i, msg[0], msg[payload_size - 1],ss[4*i+2],ss[4*i+3], ss[4*i], ss[4*i+1]);
            //     sched_yield();
            // }
            // printf("3333---%u, %c-%c\n", i, msg[0], msg[payload_size - 1]);
            end_tsc[i] = rte_rdtsc();
        }

        latency_report_perf(start_tsc, end_tsc, targv->sublink_idx);
        // for (size_t i = 0; i < num_req; ++i){
        //     printf("%c%c%c%c\n", ss[4*i], ss[4*i+1], ss[4*i+2], ss[4*i+3]);
        // }
        free(start_tsc);
        free(end_tsc);

        rp[1] = end;

        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        // printf("I am NTB_TOPO_B2B_DSD measuring ntb latency\n");
        volatile char *str = (volatile char *)sublink->sub_local_ptr;
        memset((void *)str, 'a', payload_size);

        volatile char *rp = (volatile char *)sublink->sub_remote_ptr;

        char ack[payload_size];
        memset((void *)ack, 'a', payload_size);

        char start = 's', end = 'e';
        ack[0] = ack[payload_size - 1] = end;

        // char ss[4*num_req];
        for (size_t i = 0; i < num_req; ++i)
        {
            // recv msg
            // ss[4*i] = str[0];
            // ss[4*i+1] = str[payload_size - 1];
            // rte_io_rmb();
            while (str[0] != start || str[payload_size - 1] != start)
                sched_yield();
            // printf("%u --- %c\n", i, str[payload_size - 1]);
            // printf("1111---%u, %c-%c\n", i, str[0], str[payload_size - 1]);
            str[0] = end;
            str[payload_size - 1] = end;
            // rte_io_rmb();
            // ss[4*i+2] = str[0];
            // ss[4*i+3] = str[payload_size - 1];
            // printf("2222---%u, %c-%c\n", i, str[0], str[payload_size - 1]);
            // while (str[0] != end || str[payload_size - 1] != end) sched_yield();
            // printf("3333---%u, %c-%c\n", i, str[0], str[payload_size - 1]);
            rte_memcpy((void *)rp, ack, payload_size);
            // rte_io_rmb();
        }
        // for (size_t i = 0; i < num_req; ++i){
        //     printf("%c%c%c%c\n", ss[4*i], ss[4*i+1], ss[4*i+2], ss[4*i+3]);
        // }
        while (str[1] != end)
            sched_yield();

        break;
    }

    default:
        break;
    }

    return NULL;
}

void *ntb_lat_rr(void *argv)
{
    struct thrds_argv *targv = ((struct thrds_argv *)argv);
    struct ntb_link *link = targv->link;
    struct ntb_sublink *sublink = link->sublink[targv->sublink_idx];
    pin_1thread_to_1core(targv->sublink_idx);

    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        while (!start)
            sched_yield();

        size_t *start_tsc, *end_tsc;
        start_tsc = (size_t *)malloc(sizeof(size_t) * num_req);
        end_tsc = (size_t *)malloc(sizeof(size_t) * num_req);

        char msg[payload_size];
        memset((void *)msg, 'a', payload_size);

        char *lp = (char *)sublink->sub_local_ptr;
        char *rp = (char *)sublink->sub_remote_ptr;
        memset((void *)lp, 'a', payload_size);
        lp[payload_size - 1] = '\0';

        char start = 's', end = 'e';

        // remote read msg 忽略第一个msg
        rte_memcpy((void *)msg, rp, payload_size);
        while (msg[0] != start || msg[payload_size - 1] != start)
            sched_yield();
        // reset msg
        msg[0] = msg[payload_size - 1] = end;

        for (size_t i = 0; i < num_req; ++i)
        {
            // send msg
            start_tsc[i] = rte_rdtsc();

            // remote read msg
            rte_memcpy((void *)msg, rp, payload_size);
            while (msg[0] != start || msg[payload_size - 1] != start)
                sched_yield();

            // reset msg
            msg[0] = msg[payload_size - 1] = end;

            end_tsc[i] = rte_rdtsc();
            // 1个RTT
            end_tsc[i] = end_tsc[i] + (end_tsc[i] - start_tsc[i]);
        }

        latency_report_perf(start_tsc, end_tsc, targv->sublink_idx);

        free(start_tsc);
        free(end_tsc);

        lp[0] = end;

        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        char *lp = (char *)sublink->sub_local_ptr;
        char *rp = (char *)sublink->sub_remote_ptr;
        memset((void *)lp, 'a', payload_size);
        memset((void *)rp, 'a', payload_size);

        char start = 's', end = 'e';

        lp[0] = lp[payload_size - 1] = start;

        while (rp[0] != end)
            sched_yield();

        break;
    }

    default:
        break;
    }

    return NULL;
}

void ntb_close(struct ntb_link *link, uint16_t dev_id)
{
    for (int i = 0; i < thrds; ++i)
    {
        free(link->sublink[i]);
    }
    free(link->sublink);
    free(link);
    rte_rawdev_stop(dev_id);
    rte_rawdev_close(dev_id);
}

void report_result_lat(void)
{
    // 一维表示AVERAGE/50 TAIL/99 TAIL/99.9 TAIL/99.99 TAIL
    // 二维表示MIN/AVERAGE/MAX
    double ret[5][3];

    for (int i = 0; i < 5; ++i)
    {
        ret[i][0] = result_lat[0][i];
        ret[i][1] = result_lat[0][i];
        ret[i][2] = result_lat[0][i];
    }

    for (int i = 1; i < thrds; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            if (result_lat[i][j] < ret[j][0])
                ret[j][0] = result_lat[i][j];
            if (result_lat[i][j] > ret[j][2])
                ret[j][2] = result_lat[i][j];
            ret[j][1] += result_lat[i][j];
        }
    }

    printf("\n=================== FINAL RESULT ===================\n");
    char sign[5][12] = {"AVERAGE", "50 TAIL", "99 TAIL", "99.9 TAIL", "99.99 TAIL"};
    for (int i = 0; i < 5; ++i)
    {
        printf("\nMIN %s = %.2f us\n", sign[i], ret[i][0]);
        printf("AVERAGE %s = %.2f us\n", sign[i], ret[i][1] / thrds);
        printf("MAX %s = %.2f us\n", sign[i], ret[i][2]);
    }

    for (int i = 0; i < thrds; ++i)
    {
        free(result_lat[i]);
    }
    free(result_lat);
}

void report_result_tput(void)
{
    // MIN/TOTAL/MAX
    double ret[3];

    ret[0] = ret[1] = ret[2] = result_tput[0];
    for (int i = 1; i < thrds; ++i)
    {
        if (result_tput[i] < ret[0])
            ret[0] = result_tput[i];
        if (result_tput[i] > ret[2])
            ret[2] = result_tput[i];
        ret[1] += result_tput[i];
    }

    printf("\n=================== FINAL RESULT ===================\n");
    printf("\nMIN TPUT = %.2f req/s\t\t MIN BW = %.2f Gb/s\n", ret[0], ret[0] / 1024 / 1024 / 128 * payload_size);
    printf("AVERAGE TPUT = %.2f req/s \t\t AVERAGE BW = %.2f Gb/s\n", ret[1] / thrds, ret[1] / thrds / 1024 / 1024 / 128 * payload_size);
    printf("MAX TPUT = %.2f req/s \t\t MAX BW = %.2f Gb/s\n", ret[2], ret[2] / 1024 / 1024 / 128 * payload_size);
    printf("TOTAL TPUT = %.2f req/s \t\t TOTAL BW = %.2f Gb/s\n", ret[1], ret[1] / 1024 / 1024 / 128 * payload_size);

    free(result_tput);
}

static int lcore_ntb_daemon(void)
{
    uint16_t dev_id = find_raw_ntb_device();

    if (!check_ntb_link_status(dev_id))
        return -1;

    struct ntb_link *link = raw_ntb_start(dev_id);

    if (link->hw == NULL)
        rte_exit(EXIT_FAILURE, "invalid device.");

    printf("mem addr == %ld, len == %ld, local ntb addr = %p, len = %lu, remote ntb addr = %p, len = %lu\n", link->hw->pci_dev->mem_resource[2].phys_addr, link->hw->pci_dev->mem_resource[2].len, link->local_ptr, link->hw->mz[0]->len, link->remote_ptr, link->hw->pci_dev->mem_resource[2].len);

    uint16_t reg_val;
    int ret = rte_pci_read_config(link->hw->pci_dev, &reg_val,
                                  sizeof(uint16_t), XEON_LINK_STATUS_OFFSET);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "unable to get link status.");

    link->hw->link_status = NTB_LNK_STA_ACTIVE(reg_val);
    if (link->hw->link_status)
    {
        link->hw->link_speed = NTB_LNK_STA_SPEED(reg_val);
        link->hw->link_width = NTB_LNK_STA_WIDTH(reg_val);
        printf("link_speed == %d,link_width == %d \n", link->hw->link_speed, link->hw->link_width);
    }
    DEBUG("start exchange msg...");
    ntb_connect(link);
    link->local_ptr += 128;
    link->remote_ptr += 128;

    // allocate memory for performance result array
    if (link->hw->topo == NTB_TOPO_B2B_USD)
    {
        if (metric == RW_LAT || metric == RR_LAT)
        {
            result_lat = (double **)calloc(thrds, sizeof(double *));
            for (int i = 0; i < thrds; ++i)
            {
                result_lat[i] = (double *)calloc(5, sizeof(double));
            }
        }
        else if (metric == RW_TPUT || metric == RR_TPUT)
        {
            result_tput = (double *)calloc(thrds, sizeof(double));
        }
    }

    pthread_t threads[thrds];
    struct thrds_argv targv[thrds];
    for (int i = 0; i < thrds; ++i)
    {
        targv[i].link = link;
        targv[i].sublink_idx = i;
        switch (metric)
        {
        case RW_TPUT:
            pthread_create(&threads[i], NULL, ntb_tput_rw, &targv[i]);
            break;
        case RW_LAT:
            pthread_create(&threads[i], NULL, ntb_lat_rw, &targv[i]);
            break;
        case RR_TPUT:
            pthread_create(&threads[i], NULL, ntb_tput_rr, &targv[i]);
            break;
        case RR_LAT:
            pthread_create(&threads[i], NULL, ntb_lat_rr, &targv[i]);
            break;
        default:
            rte_exit(EXIT_FAILURE, "No such metric\n");
            break;
        }
    }

    printf("sleep 1s\n");
    sleep(1);
    start = true;
    printf("start transfer...\n");
    if (metric == RW_TPUT || metric == RR_TPUT)
    {
        sleep(RUN_TIME);
        stop = true;
    }
    for (int i = 0; i < thrds; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    // print final performance result
    if (link->hw->topo == NTB_TOPO_B2B_USD)
    {
        if (metric == RW_LAT || metric == RR_LAT)
            report_result_lat();
        else if (metric == RW_TPUT || metric == RR_TPUT)
            report_result_tput();
    }

    ntb_close(link, dev_id);
    return 0;
}

bool check_ntb_link_status(uint16_t dev_id)
{
    uint64_t ntb_link_status;

    /* Waiting for peer dev up at most 100s.*/
    printf("Checking ntb link status...\n");
    for (int i = 0; i < 1000; i++)
    {
        rte_rawdev_get_attr(dev_id, NTB_LINK_STATUS_NAME,
                            &ntb_link_status);
        if (ntb_link_status)
        {
            printf("Peer dev ready, ntb link up.\n");
            return true;
        }
        rte_delay_ms(100);
    }
    rte_rawdev_get_attr(dev_id, NTB_LINK_STATUS_NAME, &ntb_link_status);
    if (ntb_link_status == 0)
    {
        printf("Expire 100s. Link is not up. Please restart app.\n");
        return false;
    }
    return true;
}

int find_raw_ntb_device(void)
{
    char raw_ntb_name[8] = "raw_ntb";
    for (int i = 0; i < RTE_RAWDEV_MAX_DEVS; ++i)
    {
        if (rte_rawdevs[i].driver_name && strncmp(rte_rawdevs[i].driver_name, raw_ntb_name, strlen(raw_ntb_name)) == 0 && rte_rawdevs[i].attached == 1)
        {
            DEBUG("find ntb device, device id = %d", i);
            return i;
        }
    }
    rte_exit(EXIT_FAILURE, "cannot find any ntb device.\n");
}

struct ntb_link *raw_ntb_start(uint16_t dev_id)
{
    struct ntb_link *link = (struct ntb_link *)malloc(sizeof(struct ntb_link));
    DEBUG("start ntb device, device id = %d", dev_id);

    struct rte_rawdev *dev = &rte_rawdevs[dev_id];
    link->dev = dev;
    link->hw = dev->dev_private;

    if (dev->started)
    {
        printf("ntb device with dev_id[%u] is alreadly started\n", dev_id);
        return 0;
    }

    int ret = (*dev->dev_ops->dev_start)(dev);
    if (ret == 0)
        dev->started = 1;

    DEBUG("ntb device has started, dev_id=%u", dev_id);
    link->local_ptr = (uint8_t *)link->hw->mz[0]->addr;
    link->remote_ptr = (uint8_t *)link->hw->pci_dev->mem_resource[2].addr;

    // 留出128字节用于传输控制信息
    link->cur_local_ptr = link->local_ptr + 128;
    link->cur_remote_ptr = link->remote_ptr + 128;

    int align_cache_line = ((payload_size + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;

    // create sublink
    link->sublink = (struct ntb_sublink **)calloc(thrds, sizeof(struct ntb_sublink *));
    for (int i = 0; i < thrds; ++i)
    {
        link->sublink[i] = (struct ntb_sublink *)calloc(1, sizeof(struct ntb_sublink));
        link->sublink[i]->sub_local_ptr = link->cur_local_ptr;
        link->sublink[i]->sub_remote_ptr = link->cur_remote_ptr;
        // printf("sublink[%d]->sub_local_ptr = %p, sublink[%d]->sub_remote_ptr = %p\n", i, link->cur_local_ptr, i, link->cur_remote_ptr);
        link->cur_local_ptr += align_cache_line;
        link->cur_remote_ptr += align_cache_line;
    }

    return link;
}

void usage(void)
{
    fprintf(stdout, "NTB-bench-tool Usage: \n");
    fprintf(stdout, " -s <size>      payload size(default %d)\n", 16);
    fprintf(stdout, " -n <requests>  the number of request(default %d)\n", 10000);
    fprintf(stdout, " -m <metric>    the method of measuring raw ntb\n");
    fprintf(stdout, "                    1 - tput(remote write), 2 - lat(remote write)\n");
    fprintf(stdout, "                    3 - tput(remote read), 4 - lat(remote read)\n");
    fprintf(stdout, " -t <threads>   the number of threads\n");
    fprintf(stdout, " -h             display the help information\n");
}

// parsing command line parameters
void parse_args(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i)
    {
        if (strlen(argv[i]) == 2 && strcmp(argv[i], "-s") == 0)
        {
            if (i + 1 < argc)
            {
                payload_size = atoi(argv[i + 1]);
                if (payload_size <= 0)
                {
                    fprintf(stdout, "invalid payload size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                fprintf(stdout, "cannot read payload size\n");
                usage();
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-n") == 0)
        {
            if (i + 1 < argc)
            {
                num_req = atoi(argv[i + 1]);
                if (num_req <= 0)
                {
                    fprintf(stdout, "invalid the number of requests\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                fprintf(stdout, "cannot read the number of requests\n");
                usage();
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-t") == 0)
        {
            if (i + 1 < argc)
            {
                thrds = atoi(argv[i + 1]);
                if (thrds < 1 || thrds > 128)
                {
                    fprintf(stderr, "invalid threads number\n");
                    exit(EXIT_FAILURE);
                }
                ++i;
            }
            else
            {
                fprintf(stderr, "cannot read threads number\n");
                usage();
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-m") == 0)
        {
            if (i + 1 < argc)
            {
                metric = atoi(argv[i + 1]);
                if (metric < 1 || metric > 6)
                {
                    fprintf(stderr, "invalid metric\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                fprintf(stderr, "cannot read metric\n");
                usage();
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-h") == 0)
        {
            usage();
            exit(EXIT_SUCCESS);
        }
    }
}

void latency_report_perf(size_t *start_tsc, size_t *end_tsc, int thrd_id)
{

    FILE *fp;

    double sum = 0.0;
    double *lat = (double *)malloc(num_req * sizeof(double));
    ;
    // printf("\nbefore sort:\n");
    for (size_t i = 0; i < num_req; i++)
    {
        lat[i] = (double)(end_tsc[i] - start_tsc[i]) / rte_get_tsc_hz() * US_PER_S;
        sum += lat[i];
    }
    qsort(lat, num_req, sizeof(double), cmp);
    size_t idx_m, idx_99, idx_99_9, idx_99_99;
    idx_m = floor(num_req * 0.5);
    idx_99 = floor(num_req * 0.99);
    idx_99_9 = floor(num_req * 0.999);
    idx_99_99 = floor(num_req * 0.9999);
    printf("@MEASUREMENT(requests = %lu, payload size = %d, threadID = %d):\n", num_req, payload_size, thrd_id);
    printf("AVERAGE = %.2f us\n50 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n99.99 TAIL = %.2f us\n", sum / num_req, lat[idx_m], lat[idx_99], lat[idx_99_9], lat[idx_99_99]);

    result_lat[thrd_id][0] = sum / num_req;
    result_lat[thrd_id][1] = lat[idx_m];
    result_lat[thrd_id][2] = lat[idx_99];
    result_lat[thrd_id][3] = lat[idx_99_9];
    result_lat[thrd_id][4] = lat[idx_99_99];

    fp = fopen("data.txt", "w");

    for (size_t i = 0; i < num_req; i++)
    {
        fprintf(fp, "%lf\n", lat[i]);
    }

    fclose(fp);

    free(lat);
}

#pragma GCC diagnostic ignored "-Wcast-qual"
int main(int argc, char *argv[])
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization.\n");

    argc -= ret;
    argv += ret;

    parse_args(argc, argv);

    lcore_function_t *func = (lcore_function_t *)lcore_ntb_daemon;
    printf("num_req=%lu, payload_size=%d, metric=%d\n", num_req, payload_size, metric);
    if (rte_eal_process_type() == RTE_PROC_PRIMARY)
    {
        rte_eal_mp_remote_launch(func, NULL, CALL_MASTER);
        unsigned lcore_id;
        RTE_LCORE_FOREACH_SLAVE(lcore_id)
        {
            if (rte_eal_wait_lcore(lcore_id) < 0)
            {
                ret = -1;
                break;
            }
        }
    }

    return 0;
}
