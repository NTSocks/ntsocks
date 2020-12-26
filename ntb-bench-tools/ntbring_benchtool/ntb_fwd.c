#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

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

#include "ntb_ring.h"
#include "nt_log.h"
#include "ntb.h"
#include "ntb_hw_intel.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);

#define RUN_TIME 10
#define END_FLAG 'e'
#define START_FLAG 's'
#define NULL_FLAG '\0'

enum METRIC
{
    RW_TPUT = 1,
    RW_LAT,
    RW_BW,
    RR_TPUT,
    RR_LAT,
    RR_BW
};

int payload_size = 16;
size_t num_req = 10000;
// 1 - tput(remote write), 2 - lat(remote write), 3 - bw(remote write)
// 4 - tput(remote read), 5 - lat(remote read), 6 - bw(remote read)
int metric = 1;

/*
 * 五种方式：
 * 1. local_ring中cus_ptr和pd_ptr均表示本地
 * 2. local_ring中cus_ptr表示本地消费指针，pd_ptr表示对端生产指针
 * 3. local_ring中cus_ptr表示对端消费指针，pd_ptr表示本地生产指针
 * 4. local_ring中cus_ptr和pd_ptr均表示对端
 * 5. local_ring中cus_ptr表示本地消费指针，remote_ring中pd_ptr表示对端生产指针，本地留出32 bits同步远程消费指针
 */
int ring_type = 1;

struct ntb_link *raw_ntb_start(uint16_t dev_id);
static bool check_ntb_link_status(uint16_t dev_id);
static int find_raw_ntb_device(void);
int static lcore_ntb_daemon(void*);
static void parse_args(int argc, char *argv[]);
static void usage(void);
static void latency_report_perf(size_t *start_tsc, size_t *end_tsc);

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
        char *str = "start";
        DEBUG("send `start` msg");
        rte_memcpy(link->remote_ptr, str, 6);

        rte_wmb();
        DEBUG("ready to recv `ack` msg");
        char *ack = (char *)link->local_ptr;
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

        char *ack = "ack";
        DEBUG("ready to send `ack` msg");
        rte_memcpy(link->remote_ptr, ack, 4);
        DEBUG("send `ack` msg");
        rte_wmb();
        break;
    }

    default:
        break;
    }
}

void ntb_tput_rw(struct ntb_link *link, bool flag)
{
    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        uint64_t prev_tsc, cur_tsc, timer_tsc = 0, old_timer_tsc = 0;
        int run_times = 1;

        uint64_t counter = 0, old_counter = 0;
        double elapse_time = 0.0;

        struct ring_item *item = (struct ring_item *)malloc(sizeof(struct ring_item));
        item->len = payload_size;
        struct ring_item *out_item = (struct ring_item *)malloc(sizeof(struct ring_item));
        out_item->len = 1;
        out_item->data = (char *)malloc(1);

        int ret = 1;
        char msg[payload_size];
        memset(msg, 'a', payload_size);
        msg[payload_size - 1] = '\0';
        item->data = msg;

        while (1)
        {
            ret = 1;
            prev_tsc = rte_rdtsc();

            while (1)
            {
                ret = link->ring_enqueue(link, item);
                if (ret == 0)
                    break;
            }

            ret = 1;
            while (1)
            {
                ret = link->ring_dequeue(link, out_item);
                if (ret == 0)
                {
                    break;
                }
            }

            cur_tsc = rte_rdtsc();

            counter++;
            timer_tsc += (cur_tsc - prev_tsc);
            if (unlikely(timer_tsc >= run_times * rte_get_timer_hz()))
            {
                run_times++;

                elapse_time = (double)(timer_tsc - old_timer_tsc) / rte_get_timer_hz();

                printf("[remote write] ring_type = %d, elapse_time = %.2lf, loop counter = %ld ; TPUT (ops/s) = %.2lf \n", ring_type, elapse_time, counter - old_counter, (double)(counter - old_counter) / elapse_time);

                old_timer_tsc = timer_tsc;
                old_counter = counter;

                if (run_times > RUN_TIME)
                {
                    link->remote_ptr[0] = END_FLAG;
                    break;
                }
            }
        }

        elapse_time = (double)timer_tsc / rte_get_timer_hz();

        free(item);
        free(out_item->data);
        free(out_item);

        printf("[remote write] ring_type = %d, send buff = %d, total elapse_time = %.2lf, loop counter = %ld ; TPUT (ops/s) = %.2lf \n", ring_type, payload_size, elapse_time, counter, (double)counter / elapse_time);

        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        struct ring_item *out_item = (struct ring_item *)malloc(sizeof(struct ring_item));
        out_item->data = (char *)malloc(payload_size);
        memset(out_item->data, '\0', payload_size);

        struct ring_item *in_item = (struct ring_item *)malloc(sizeof(struct ring_item));
        in_item->data = "b\0";
        in_item->len = 1;

        int ret = 1;
        while (1)
        {
            // recv msg
            ret = 1;
            while (1)
            {
                ret = link->ring_dequeue(link, out_item);
                if (ret == 0)
                    break;
                if (unlikely(link->local_ptr[0] == END_FLAG))
                    goto EXIT;
            }

            // send ack msg
            ret = 1;
            while (1)
            {
                ret = link->ring_enqueue(link, in_item);
                if (ret == 0)
                    break;
            }
        }

    EXIT:

        free(out_item->data);
        free(out_item);
        free(in_item);

        break;
    }

    default:
        break;
    }
}

void ntb_lat_rw(struct ntb_link *link)
{
    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        // printf("I am NTB_TOPO_B2B_USD measuring ntb latency\n");
        size_t *start_tsc, *end_tsc;

        start_tsc = (size_t *)malloc(sizeof(size_t) * num_req);
        end_tsc = (size_t *)malloc(sizeof(size_t) * num_req);

        struct ring_item *item = (struct ring_item *)malloc(sizeof(struct ring_item));
        item->len = payload_size;

        int ret = 1;
        char msg[payload_size];
        memset(msg, 'a', payload_size);
        msg[payload_size - 1] = '\0';
        item->data = msg;

        struct ring_item *ack_item = (struct ring_item *)malloc(sizeof(struct ring_item));
        ack_item->data = (char *)malloc(payload_size);

        for (size_t i = 0; i < num_req; ++i)
        {
            start_tsc[i] = rte_rdtsc();

            // send msg
            ret = 1;
            while (1)
            {
                ret = link->ring_enqueue(link, item);
                if (ret == 0)
                    break;
            }

            // recv ack msg
            ret = 1;
            while (1)
            {
                ret = link->ring_dequeue(link, ack_item);
                if (ret == 0)
                {
                    // if (strcmp(item->data, msg) != 0){
                    //     fprintf(stdout, "recv ack  failed!\n");
                    //     exit(EXIT_FAILURE);
                    // }
                    break;
                }
            }

            end_tsc[i] = rte_rdtsc();
        }

        latency_report_perf(start_tsc, end_tsc);
        link->remote_ptr[0] = END_FLAG;

        free(item);
        free(ack_item->data);
        free(ack_item);

        free(start_tsc);
        free(end_tsc);

        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        // printf("I am NTB_TOPO_B2B_DSD measuring ntb latency\n");

        struct ring_item *item = (struct ring_item *)malloc(sizeof(struct ring_item));
        item->data = (char *)malloc(payload_size);

        struct ring_item *ack_item = (struct ring_item *)malloc(sizeof(struct ring_item));
        ack_item->len = payload_size;

        char msg[payload_size];
        memset(msg, 'k', payload_size);
        msg[payload_size - 1] = '\0';
        ack_item->data = msg;

        int ret = 1;

        for (size_t i = 0; i < num_req; ++i)
        {

            // recv msg
            ret = 1;
            while (1)
            {
                ret = link->ring_dequeue(link, item);
                if (ret == 0)
                    break;
            }

            // send ack msg
            ret = 1;
            while (1)
            {
                ret = link->ring_enqueue(link, ack_item);
                if (ret == 0)
                    break;
            }
        }
        while (link->local_ptr[0] != END_FLAG)
            sched_yield();
        free(item->data);
        free(item);
        free(ack_item);

        break;
    }

    default:
        break;
    }
}

void ntb_bw_rw(struct ntb_link *link)
{
    switch (link->hw->topo)
    {

    case NTB_TOPO_B2B_USD:
    {
        DEBUG("I'm NTB_TOPO_B2B_USD");
        struct ring_item *item = (struct ring_item *)malloc(sizeof(struct ring_item));
        item->len = payload_size;

        char msg[payload_size];
        memset(msg, 'a', payload_size);
        msg[payload_size - 1] = '\0';

        // 如果定义为int，有可能会溢出
        int ret = 1;

        while (1)
        {
            // send message
            item->data = msg;
            ret = 1;
            while (1)
            {
                ret = link->ring_enqueue(link, item);
                if (ret == 0)
                    break;
                if (unlikely(link->local_ptr[0] == END_FLAG))
                    goto EXIT;
            }
        }

    EXIT:

        free(item);
        break;
    }

    case NTB_TOPO_B2B_DSD:
    {
        DEBUG("I'm NTB_TOPO_B2B_DSD");
        uint64_t prev_tsc, cur_tsc, timer_tsc = 0, old_timer_tsc = 0;
        int run_times = 1;
        double elapse_time = 0.0;
        uint64_t counter = 0, old_counter = 0;

        struct ring_item *item = (struct ring_item *)malloc(sizeof(struct ring_item));
        item->data = malloc(payload_size);
        memset(item->data, 'a', payload_size);
        item->len = payload_size;

        // 第一个msg不计入
        int ret = 1;
        while (1)
        {
            ret = link->ring_dequeue(link, item);
            if (ret == 0)
                break;
        }

        while (1)
        {
            prev_tsc = rte_rdtsc();

            ret = 1;
            while (1)
            {
                ret = link->ring_dequeue(link, item);
                if (ret == 0)
                    break;
            }

            cur_tsc = rte_rdtsc();
            counter++;
            timer_tsc += (cur_tsc - prev_tsc);

            if (unlikely(timer_tsc >= run_times * rte_get_timer_hz()))
            {
                run_times++;
                elapse_time = (double)(timer_tsc - old_timer_tsc) / rte_get_timer_hz();
                printf("[remote write] elapse_time = %.2lf, loop counter = %ld ; BW (Gbits/s) = %.2lf \n", elapse_time, counter - old_counter, (double)(counter - old_counter) / (elapse_time * 128 * 1024 * 1024) * payload_size);

                old_timer_tsc = timer_tsc;
                old_counter = counter;

                if (run_times > RUN_TIME)
                    break;
            }
        }

        elapse_time = (double)timer_tsc / rte_get_timer_hz();
        printf("[remote write] send buff = %d, total elapse_time = %.2lf, loop counter = %ld ; BW (Gbits/s) = %.2lf \n", payload_size, elapse_time, counter, (double)counter / (elapse_time * 128 * 1024 * 1024) * payload_size);

        link->remote_ptr[0] = END_FLAG;
        break;
    }

    default:
        break;
    }
}

void ntb_close(struct ntb_link *link, uint16_t dev_id)
{
    free(link->local_ring);
    free(link->remote_ring);
    free(link);

    rte_rawdev_stop(dev_id);
    rte_rawdev_close(dev_id);
}

int lcore_ntb_daemon(void* arg)
{
    uint16_t dev_id = find_raw_ntb_device();
    if (!check_ntb_link_status(dev_id))
        return -1;
    struct ntb_link *link = raw_ntb_start(dev_id);

    if (link->hw == NULL)
        rte_exit(EXIT_FAILURE, "invalid device.");

    printf("local ntb addr = %p, len = %lu, remote ntb addr = %p, len = %lu\n",
           link->local_ptr, link->hw->mz[0]->len, link->remote_ptr, link->hw->pci_dev->mem_resource[2].len);

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

    // 留出128位防止数据被覆盖，导致对端没来得及接收数据
    link->cur_local_ptr = link->local_ptr + 128;
    link->cur_remote_ptr = link->remote_ptr + 128;
    ring_init(link, num_req, payload_size, ring_type);

    uint64_t prev_tsc, cur_tsc;

    ntb_connect(link);
    memset(link->local_ptr, 0, 128);

    prev_tsc = rte_rdtsc();
    DEBUG("start exchange msg...");
    ntb_connect(link);
    cur_tsc = rte_rdtsc();
    printf("ntb connect time is %f us\n", (double)(cur_tsc - prev_tsc) / rte_get_tsc_hz() * US_PER_S);

    switch (metric)
    {
    case RW_TPUT:
        ntb_tput_rw(link, true);
        break;
    case RW_LAT:
        ntb_lat_rw(link);
        break;
    case RW_BW:
        ntb_bw_rw(link);
        break;
    default:
        rte_exit(EXIT_FAILURE, "No such metric\n");
        break;
    }
    ntb_close(link, dev_id);

    return 0;
}

int find_raw_ntb_device(void)
{
    char raw_ntb_name[8] = "raw_ntb";
    for (int i = 0; i < RTE_RAWDEV_MAX_DEVS; ++i)
    {
        if (rte_rawdevs[i].driver_name && strncmp(rte_rawdevs[i].driver_name, raw_ntb_name, strlen(raw_ntb_name)) == 0 &&
            rte_rawdevs[i].attached == 1)
            DEBUG("find ntb device, device id = %d", i);
        return i;
    }
    rte_exit(EXIT_FAILURE, "cannot find any ntb device.\n");
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

    link->cur_local_ptr = link->local_ptr = (uint8_t *)link->hw->mz[0]->addr;
    link->cur_remote_ptr = link->remote_ptr = (uint8_t *)link->hw->pci_dev->mem_resource[2].addr;

    link->local_cus_ptr = link->remote_pd_ptr = 0;

    DEBUG("ntb device has started, link->local_ptr=%p, link->remote_ptr=%p", link->local_ptr, link->remote_ptr);

    return link;
}

void usage(void)
{
    fprintf(stdout, "NTB Usage: \n");
    fprintf(stdout, " -s <size>      payload size(default %d)\n", 16);
    fprintf(stdout, " -n <requests>  the number of request(default %d)\n", 10000);
    fprintf(stdout, " -m <metric>    the method of measuring raw ntb\n");
    fprintf(stdout, "                    1 - tput(remote write), 2 - lat(remote write), 3 - bw(remote write)\n");
    fprintf(stdout, " -r <ring_type> the ring type of raw ntb\n");
    fprintf(stdout, "                    1 - local cus ptr and local pd ptr\n");
    fprintf(stdout, "                    2 - local cus ptr and remote pd ptr\n");
    fprintf(stdout, "                    3 - remote cus ptr and local pd ptr\n");
    fprintf(stdout, "                    4 - remote cus ptr and remote pd ptr\n");
    fprintf(stdout, "                    5 - local cus ptr(OPTIMAL)\n");
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
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-r") == 0)
        {
            if (i + 1 < argc)
            {
                ring_type = atoi(argv[i + 1]);
                if (ring_type < 1 || ring_type > 5)
                {
                    fprintf(stderr, "invalid ring type\n");
                    exit(EXIT_FAILURE);
                }
                ++i;
            }
            else
            {
                fprintf(stderr, "cannot read ring type\n");
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

void latency_report_perf(size_t *start_tsc, size_t *end_tsc)
{
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
    fprintf(stdout, "@MEASUREMENT(requests = %lu, payload size = %d):\n", num_req, payload_size);
    fprintf(stdout, "AVERAGE = %.2f us\n50 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n99.99 TAIL = %.2f us\n", sum / num_req, lat[idx_m], lat[idx_99], lat[idx_99_9], lat[idx_99_99]);
    free(lat);
}

int main(int argc, char *argv[])
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization.\n");

    argc -= ret;
    argv += ret;

    parse_args(argc, argv);
    printf("num_req=%lu, payload_size=%d, metric=%d\n", num_req, payload_size, metric);
    if (rte_eal_process_type() == RTE_PROC_PRIMARY)
    {
        rte_eal_mp_remote_launch(lcore_ntb_daemon, NULL, CALL_MASTER);
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
