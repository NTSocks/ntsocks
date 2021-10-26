
#include <arpa/inet.h>
#include "../common/common.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
int num_req = NUM_REQ;
bool with_ack = false;
int thrds = 1;
bool waiting_transfer_data = true;
int run_latency = 0; // 0 - lat, 1 - tput, 2 - bw
int payload_size = DEFAULT_PAYLOAD_SIZE;
bool to_file = false;
int partition = 1;
int packet_size = 128;
char file_path[256];
char file_name[128];
int time_in_seconds = 5;
int numa_node_type = 2;
int monitoring = 1;
int running = 1;

pthread_mutex_t mutex;

static int cmp(const void *a, const void *b)
{
    return *(double *)a > *(double *)b ? 1 : -1;
}

long long read_persecond[MAX_NUM_THREADS] = {0};

void usage(char *program);
void parse_args(int argc, char *argv[]);
void *handle_connection(void *ptr);
void latency_report_perf(size_t *start_cycles, size_t *end_cycles, int sockfd);
void latency_report_perf_to_file(size_t *start_cycles, size_t *end_cycles, int sockfd);
void latency_write(int sockfd, size_t *start_cycles, size_t *end_cycles);
void latency_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles);
void bandwidth_read(struct conn_ctx *ctx);
void *monitor_bandwidth();
void *monitor_bandwidth_update();
void throughput_read_with_ack(struct conn_ctx *ctx);
void throughput_read_with_ack_in_time(struct conn_ctx *ctx);

uint64_t Now64()
{
    struct timespec tv;
    int res = clock_gettime(CLOCK_REALTIME, &tv);
    return (uint64_t)tv.tv_sec * 1000000llu + (uint64_t)tv.tv_nsec / 1000;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, 0, _IONBF, 0);

    int listen_fd, sockfd;
    struct sockaddr_in address, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    pthread_t serv_thread[MAX_NUM_THREADS];
    struct conn_ctx conns[MAX_NUM_THREADS];
    pthread_t monitor_thead;
    int last_core = 0;
    long cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);

    // read command line arguments
    parse_args(argc, argv);

    /** 
     * numa_node_type is 0: the even-numbered core is the numa0 node, 
     *  and the odd-numbered core is the numa1 node. 
     *  The network card is in the numa0 node, 
     *  and we start the allocation from core 0.
     * numa_node_type is 1:  get core from 16-31,48-63
     * numa_node_type is 2:  get core from 0-15,32-47
     */
    if (numa_node_type == 0)
    {
        // The number of cores of the machine is always even， eg, 24,64
        last_core = 0;
    }
    else if (numa_node_type == 1)
    {
        last_core = 16;
    }
    else if (numa_node_type == 2)
    {
        last_core = 48;
    }

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons(server_port);
    printf("bind to %s:%d\n", server_ip, server_port);
    if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)))
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, SOCK_BACKLOG_CONN) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("server listens on 0.0.0.0:%d\n", server_port);

    if (to_file)
    {
        sprintf(file_name, "benchmark_%d_%d_%d_%d_%d_%d.txt",
                run_latency, payload_size, num_req, thrds, partition, packet_size);
        strcat(file_path, file_name);
    }

    for (int i = 0; i < thrds; ++i)
    {
        printf("thread:%d, waiting for accept\n", i);
        sockfd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
        conns[i].id = i;
        conns[i].sockfd = sockfd;
        if (numa_node_type == 0)
        {
            conns[i].cpumask = get_core_id(&last_core, cpu_cores, 0);
        }
        else if (numa_node_type == 1)
        {
            conns[i].cpumask = get_core_id2(&last_core, 0);
        }
        else if (numa_node_type == 2)
        {
            conns[i].cpumask = get_core_id3(&last_core, 1);
        }
        if (sockfd < 0)
        {
            perror("accept failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }
        else if (pthread_create(&serv_thread[i],
                                NULL, handle_connection, (void *)&conns[i]) < 0)
        {
            close(listen_fd);
            perror("Error: create pthread");
        }
    }

    // When test in bandwidth case, create a thread to
    //  monitor the amount of data transferred per second
    if (run_latency == 1)
    {
        time_in_seconds += 2; // The server runs for two more seconds
        pthread_create(&monitor_thead, NULL, monitor_bandwidth_update, NULL);
    }

    printf("going to sleep 1s\n");
    sleep(1); // wait 1s
    waiting_transfer_data = false;

    // If we test throughput in time limit case,
    //  the server will block at read message from client
    //  when the client has reached the test time.
    // So we need to exit the program at server after reached the test time.
    if (run_latency == 3)
    {
        sleep(time_in_seconds);
        exit(1);
    }
    else
    {
        // For other test case, we need to wait
        //  for all worker threads to finish
        for (int i = 0; i < thrds; ++i)
        {
            pthread_join(serv_thread[i], NULL);
        }

        // When we test in bandwidth case,
        //  we need to wait for monitor_thead to finish
        if (run_latency == 1)
            pthread_join(monitor_thead, NULL);
    }

    printf("server done!\n");
    close(listen_fd);

    return 0;
}

void *handle_connection(void *ptr)
{
    struct conn_ctx *ctx;
    ctx = (struct conn_ctx *)ptr;
    int sockfd = ctx->sockfd;
    printf("waiting for transfer data on %s:%d\n", server_ip, sockfd);
    if (thrds > 0)
    {
        pin_1thread_to_1core(ctx->cpumask);
        // wait for starting transfer data
        while (waiting_transfer_data)
            sched_yield();
    }
    printf("start transfer data\n");

    // Determine which function to run based on the parameters
    if (run_latency == 0)
    {
        size_t *start_cycles, *end_cycles;
        start_cycles = (size_t *)malloc(sizeof(size_t) * num_req);
        end_cycles = (size_t *)malloc(sizeof(size_t) * num_req);
        if (with_ack)
        {
            latency_write_with_ack(sockfd, start_cycles, end_cycles);
        }
        else
        {
            latency_write(sockfd, start_cycles, end_cycles);
        }
        if (to_file)
        {
            latency_report_perf_to_file(start_cycles, end_cycles, sockfd);
        }
        else
        {
            latency_report_perf(start_cycles, end_cycles, sockfd);
        }
        free(start_cycles);
        free(end_cycles);
    }
    else if (run_latency == 1)
    {
        bandwidth_read(ctx);
    }
    else if (run_latency == 2)
    {
        throughput_read_with_ack(ctx);
    }
    else if (run_latency == 3)
    {
        throughput_read_with_ack_in_time(ctx);
    }

    fflush(stdout);
    return (void *)0;
}

// measure latency without ack after writing message
void latency_write(int sockfd, size_t *start_cycles, size_t *end_cycles)
{
    char msg[payload_size];
    for (size_t i = 0; i < num_req; i++)
    {
        start_cycles[i] = get_cycles();
        write(sockfd, msg, payload_size);
        end_cycles[i] = get_cycles();
    }
}

// measure latency with reading ack after writing message
void latency_write_with_ack(int sockfd,
                            size_t *start_cycles, size_t *end_cycles)
{
    char msg[payload_size];
    char ack[payload_size];
    int n = 0;
    int ret;

    usleep(50);

    // Discard first 5000 data because of the cold start
    for (size_t i = 0; i < 5000; ++i)
    {
        ret = write(sockfd, msg, payload_size);
        if (ret <= 0)
            printf("[sockid = %d] ret = %d WRITE FAILED!!!!!!!!!!\n", sockfd, ret);
        n = payload_size;
        while (n > 0)
        {
            n = (n - read(sockfd, ack, n));
        }
    }

     for (size_t i = 0; i < num_req; ++i)
    {
        start_cycles[i] = get_cycles();

        ret = write(sockfd, msg, payload_size);
        if (ret <= 0)
            printf("[sockid = %d] ret = %d WRITE FAILED!!!!!!!!!!\n", sockfd, ret);

        n = payload_size;
        while (n > 0)
        {
            n = (n - read(sockfd, ack, n));
        }
        end_cycles[i] = get_cycles();
    }
}
int Nread(int fd, char *buf, size_t count)
{
    register ssize_t r;
    register size_t nleft = count;

    while (nleft > 0)
    {
        r = read(fd, buf, nleft);
        if (r < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return -2;
        }
        else if (r == 0)
            break;

        nleft -= r;
        buf += r;
    }
    return count - nleft;
}

void *monitor_bandwidth_update()
{
    //  Data(Gbytes) transferred per second per thread
    double bandwidth[MAX_NUM_THREADS][MAX_TIME_IN_BANDWIDTH];
    int seconds = 0;
    int real = 0;                      // The number of non-zero elements in the bandwidth array
    double *totalbandwidth_eachsecond; // requests transmitted by all threads in every second
    double allbandwidth = 0;           // All data(Gbytes) transferred
    FILE *fp;
    time_t tmpcal_ptr;
    // int step_size = 4;
    tmpcal_ptr = time(NULL) + time_in_seconds;

    uint64_t start_time, end_time;
    long long bytes_cnt_start[MAX_NUM_THREADS];
    long long bytes_cnt_end[MAX_NUM_THREADS];

    printf("begin measure bandwidth\n");
    while (time(NULL) < tmpcal_ptr)
    {
        // first time to record real-time BW
        start_time = Now64();
        for (int thr_id = 0; thr_id < thrds; thr_id++)
        {
            bytes_cnt_start[thr_id] = read_persecond[thr_id];
        }

        sleep(1);

        // second time to record real-time BW
        end_time = Now64();
        for (int thr_id = 0; thr_id < thrds; thr_id++)
        {
            bytes_cnt_end[thr_id] = read_persecond[thr_id];
        }

        if (seconds < MAX_TIME_IN_BANDWIDTH)
        {
            // caculate BW
            for (int thr_id = 0; thr_id < thrds; thr_id++)
            {
                bandwidth[thr_id][seconds] =
                    1.0 * (bytes_cnt_end[thr_id] - bytes_cnt_start[thr_id]) * 1000000 * 8 /
                    (end_time - start_time) / 1024 / 1024 / 1024;
                if (DEBUG)
                {
                    printf("thread %d bandwidth is %lf Gbits/sec, time diff is %ld\n",
                           thr_id, bandwidth[thr_id][seconds], end_time - start_time);
                }
            }
        }

        seconds++;
    }

    // if seconds > MAX_TIME_IN_BANDWIDTH, use MAX_TIME_IN_BANDWIDTH
    seconds = seconds < MAX_TIME_IN_BANDWIDTH
                  ? seconds
                  : MAX_TIME_IN_BANDWIDTH;

    // Count the amount of data transferred by all threads per second
    totalbandwidth_eachsecond = (double *)malloc(seconds * sizeof(double));
    for (int second = 0; second < seconds; second++)
    {
        for (int threadId = 0; threadId < thrds; threadId++)
        {
            totalbandwidth_eachsecond[second] += bandwidth[threadId][second];
        }
    }

    qsort(totalbandwidth_eachsecond, seconds, sizeof(double), cmp);
    if (to_file)
    {
        fp = fopen(file_path, "w");
    }
    for (int i = 0; i < seconds; i++)
    {
        if (DEBUG)
        {
            if (to_file)
            {
                fprintf(fp, "in second %d, bandwidth is %lf Gbits/sec\n",
                        i, totalbandwidth_eachsecond[i]);
            }
            else
            {
                printf("in second %d, bandwidth is %lf Gbits/sec\n",
                       i, totalbandwidth_eachsecond[i]);
            }
        }
        if (totalbandwidth_eachsecond[i] != 0)
        {
            allbandwidth += totalbandwidth_eachsecond[i];
            real++;
        }
    }
    if (to_file)
    {
        fprintf(fp, "average bandwidth is %lf Gbits/sec\n", allbandwidth / real);
        fprintf(fp, "medium bandwidth is %lf Gbits/sec\n",
                totalbandwidth_eachsecond[seconds / 2]);
        fprintf(fp, "max bandwidth is %lf Gbits/sec\n",
                totalbandwidth_eachsecond[seconds - 1]);
        fclose(fp);
    }
    else
    {
        printf("average bandwidth is %lf Gbits/sec\n", allbandwidth / real);
        printf("medium bandwidth is %lf Gbits/sec\n",
               totalbandwidth_eachsecond[seconds / 2]);
        printf("max bandwidth is %lf Gbits/sec\n",
               totalbandwidth_eachsecond[seconds - 1]);
    }
    free(totalbandwidth_eachsecond);
    exit(1);
    return (void *)0;
}

void *monitor_bandwidth()
{
    //  Data(Gbytes) transferred per second per thread
    double bandwidth[MAX_NUM_THREADS][MAX_TIME_IN_BANDWIDTH];
    int seconds = 0;
    // The number of non-zero elements in the bandwidth array
    int real = 0;
    // requests transmitted by all threads in every second
    double *totalbandwidth_eachsecond;
    double allbandwidth = 0; // All data(Gbytes) transferred
    FILE *fp;
    long long temp[MAX_NUM_THREADS];
    time_t tmpcal_ptr;
    // int step_size = 4;
    tmpcal_ptr = time(NULL) + time_in_seconds;

    // us-level
    struct timeval start, end, duration, laststart;
    double time_diff;

    printf("begin measure bandwidth\n");
    gettimeofday(&start, NULL);
    while (time(NULL) < tmpcal_ptr)
    {
        sleep(1);
        gettimeofday(&end, NULL);

        for (int threadId = 0; threadId < thrds; threadId++)
        {
            temp[threadId] = read_persecond[threadId];
            read_persecond[threadId] = 0;
        }
        laststart = start;
        gettimeofday(&start, NULL);
        timersub(&end, &laststart, &duration);
        time_diff = duration.tv_sec + (1.0 * duration.tv_usec) / 1000000;
        for (int threadId = 0; threadId < thrds; threadId++)
        {
            bandwidth[threadId][seconds] = 1.0 * temp[threadId] / 1024 / 1024 / 1024 / (time_diff);
            if (DEBUG)
            {
                printf("thread %d bandwidth is %lf Gbits/sec, time diff is %lf\n",
                       threadId, bandwidth[threadId][seconds] * 8, time_diff);
            }
            // read_persecond[threadId] = 0;
        }

        seconds++;
    }

    // Count the amount of data transferred by all threads per second
    totalbandwidth_eachsecond = (double *)malloc(seconds * sizeof(double));
    for (int second = 0; second < seconds; second++)
    {
        for (int threadId = 0; threadId < thrds; threadId++)
        {
            totalbandwidth_eachsecond[second] += bandwidth[threadId][second];
        }
    }

    qsort(totalbandwidth_eachsecond, seconds, sizeof(double), cmp);
    if (to_file)
    {
        fp = fopen(file_path, "w");
    }
    for (int i = 0; i < seconds; i++)
    {
        if (DEBUG)
        {
            if (to_file)
            {
                fprintf(fp, "in second %d, bandwidth is %lf Gbits/sec\n",
                        i, totalbandwidth_eachsecond[i] * 8);
            }
            else
            {
                printf("in second %d, bandwidth is %lf Gbits/sec\n",
                       i, totalbandwidth_eachsecond[i] * 8);
            }
        }
        if (totalbandwidth_eachsecond[i] != 0)
        {
            allbandwidth += totalbandwidth_eachsecond[i];
            real++;
        }
    }
    if (to_file)
    {
        fprintf(fp, "average bandwidth is %lf Gbits/sec\n", allbandwidth / real * 8);
        fprintf(fp, "medium bandwidth is %lf Gbits/sec\n",
                totalbandwidth_eachsecond[seconds / 2] * 8);
        fprintf(fp, "max bandwidth is %lf Gbits/sec\n",
                totalbandwidth_eachsecond[seconds - 1] * 8);
        fclose(fp);
    }
    else
    {
        printf("average bandwidth is %lf Gbits/sec\n", allbandwidth / real * 8);
        printf("medium bandwidth is %lf Gbits/sec\n",
               totalbandwidth_eachsecond[seconds / 2] * 8);
        printf("max bandwidth is %lf Gbits/sec\n",
               totalbandwidth_eachsecond[seconds - 1] * 8);
    }
    free(totalbandwidth_eachsecond);
    exit(1);
    return (void *)0;
}

void bandwidth_read(struct conn_ctx *ctx)
{
    char msg[payload_size];
    int readsize;

    //usleep(100);

    while (1)
    {
        readsize = Nread(ctx->sockfd, msg, payload_size);

        read_persecond[ctx->id] += readsize;
    }
}

void throughput_read_with_ack(struct conn_ctx *ctx)
{
    char msg[payload_size];
    char ack = '1';
    int n;

    //while (time(NULL) < tmpcal_ptr)
    for (size_t i = 0; i < num_req; ++i)
    {
        n = Nread(ctx->sockfd, msg, payload_size);
        if (n < 0)
        {
            printf("read error, n is %d\n", n);
        }
        n = write(ctx->sockfd, &ack, sizeof(ack));
        if (n < 0)
        {
            printf("write error, n is %d\n", n);
        }
    }
}

void throughput_read_with_ack_in_time(struct conn_ctx *ctx)
{
    char msg[payload_size];
    char ack = '1';
    int n;
    //int i = 1;

    //while (time(NULL) < tmpcal_ptr)
    while (running)
    {
        n = Nread(ctx->sockfd, msg, payload_size);
        if (n < 0)
        {
            printf("read error, n is %d\n", n);
        }
        //printf("Nread pass i = %d\n", i);
        n = write(ctx->sockfd, &ack, sizeof(ack));
        if (n < 0)
        {
            printf("write error, n is %d\n", n);
        }
        // printf("Send Ack pass i = %d\n", i);
        // i++;
    }
}

// print latency performance data
void latency_report_perf(size_t *start_cycles,
                         size_t *end_cycles, int sockfd)
{
    double *lat = (double *)malloc(num_req * sizeof(double));
    double cpu_mhz = get_cpu_mhz();
    double sum = 0.0;
    // printf("\nbefore sort:\n");
    for (size_t i = 0; i < num_req; i++)
    {
        lat[i] = (double)(end_cycles[i] - start_cycles[i]) / cpu_mhz;
        sum += lat[i];
    }
    qsort(lat, num_req, sizeof(lat[0]), cmp);
    size_t idx_50, idx_90, idx_99, idx_99_9;
    idx_50 = floor(num_req * 0.5);
    idx_90 = floor(num_req * 0.9);
    idx_99 = floor(num_req * 0.99);
    idx_99_9 = floor(num_req * 0.999);

    printf("@MEASUREMENT(requests = %d, payload size = %d, sockfd = %d):\n\
AVERAGE = %.2f us\n50 TAIL = %.2f us\n90 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n",
           num_req, payload_size, sockfd, sum / num_req, lat[idx_50],
           lat[idx_90], lat[idx_99], lat[idx_99_9]);

    free(lat);
}

void latency_report_perf_to_file(
    size_t *start_cycles, size_t *end_cycles, int sockfd)
{
    double *lat = (double *)malloc(num_req * sizeof(double));
    double cpu_mhz = get_cpu_mhz();
    double sum = 0.0;
    // printf("\nbefore sort:\n");
    for (size_t i = 0; i < num_req; i++) // Discard first 5000 data because of the cold start
    {
        lat[i] = (double)(end_cycles[i] - start_cycles[i]) / cpu_mhz;
        sum += lat[i];
    }

    qsort(lat, num_req, sizeof(lat[0]), cmp);
    size_t idx_50, idx_90, idx_99, idx_99_9;
    idx_50 = floor(num_req * 0.5);
    idx_90 = floor(num_req * 0.9);
    idx_99 = floor(num_req * 0.99);
    idx_99_9 = floor(num_req * 0.999);

    // Lock
    pthread_mutex_lock(&mutex);

    log_ctx_t ctx = log_init(file_path, strlen(file_path));

        fprintf(ctx->file, "@MEASUREMENT(requests = %d, payload size = %d, sockfd = %d):\n\
AVERAGE = %.2f us\n50 TAIL = %.2f us\n90 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n",
                num_req, payload_size, sockfd, sum / num_req, lat[idx_50],
                lat[idx_90], lat[idx_99], lat[idx_99_9]);

    log_destroy(ctx);

    // Unlock
    pthread_mutex_unlock(&mutex);

    free(lat);

    // // CDF
    // FILE *fp = fopen("data.txt", "w");
    // for (size_t i = 0; i < num_req; i++)
    // {
    //     fprintf(fp, "%lf\n", lat[i]);
    // }

    // fclose(fp);

    // free(lat);
}

void usage(char *program)
{
    printf("Usage: \n");
    printf("%s\tready to connect %s:%d\n", program, server_ip, server_port);
    printf("Options:\n");
    printf(" -a <addr>      connect to server addr(default %s)\n", DEFAULT_SERVER_ADDR);
    printf(" -p <port>      connect to port number(default %d)\n", DEFAULT_PORT);
    printf(" -t <threads>   handle connection with multi-threads\n");
    printf(" -s <size>      payload size(default %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf(" -n <requests>  the number of request(default %d)\n", NUM_REQ);
    printf(" -l <metric>    0 - lat, 1 - bw, 2 - tput in req_num limit, 3 - tput in time limit\n");
    printf(" -f <filepath>      the file path to save the data\n");
    printf(" -r <partition>     partition\n");
    printf(" -c <packetsize>    packet size\n");
    printf(" -w             transfer data with ack\n");
    printf(" -l             run the lantency benchmark\n");
    printf(" -h             display the help information\n");
    printf(" -d <duration>        time to measure bandwidth\n");
    printf(" -m <numa node type>       Set the way to bind core, \
0 - get core from 0,2,4,6,8.... 1 - get core from 16-31,48-63. \
2 -get core from 0-15,32-47.(default %d)\n",
           numa_node_type);
}

// parsing command line parameters
void parse_args(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (strlen(argv[i]) == 2 && strcmp(argv[i], "-p") == 0)
        {
            if (i + 1 < argc)
            {
                server_port = atoi(argv[i + 1]);
                if (server_port < 0 || server_port > 65535)
                {
                    printf("invalid port number\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read port number\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-a") == 0)
        {
            if (i + 1 < argc)
            {
                server_ip = argv[i + 1];
                i++;
            }
            else
            {
                printf("cannot read server addr\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-s") == 0)
        {
            if (i + 1 < argc)
            {
                payload_size = atoi(argv[i + 1]);
                if (payload_size <= 0)
                {
                    printf("invalid payload size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read payload size\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-w") == 0)
        {
            with_ack = true;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-f") == 0)
        {
            to_file = true;
            if (i + 1 < argc)
            {
                strcpy(file_path, argv[i + 1]);
                i++;
            }
            else
            {
                printf("cannot read file path\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-t") == 0)
        {
            if (i + 1 < argc)
            {
                thrds = atoi(argv[i + 1]);
                if (thrds <= 0 || thrds > MAX_NUM_THREADS)
                {
                    printf("invalid numbers of thread\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read numbers of thread\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-r") == 0)
        {
            if (i + 1 < argc)
            {
                partition = atoi(argv[i + 1]);
                if (partition <= 0)
                {
                    printf("invalid partition number\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read partition number\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-c") == 0)
        {
            if (i + 1 < argc)
            {
                packet_size = atoi(argv[i + 1]);
                if (packet_size <= 0)
                {
                    printf("invalid packet size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read packet size\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-d") == 0)
        {
            if (i + 1 < argc)
            {
                time_in_seconds = atoi(argv[i + 1]);
                if (time_in_seconds <= 0)
                {
                    printf("invalid time_in_seconds\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read time_in_seconds\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-l") == 0)
        {
            if (i + 1 < argc)
            {
                run_latency = atoi(argv[i + 1]);
                if (run_latency != 0 && run_latency != 1 &&
                    run_latency != 2 && run_latency != 3)
                {
                    printf("invalid metric\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read metric\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-m") == 0)
        {
            if (i + 1 < argc)
            {
                numa_node_type = atoi(argv[i + 1]);
                if (numa_node_type != 0 &&
                    numa_node_type != 1 && numa_node_type != 2)
                {
                    printf("invalid num_node type, must be 0 or 1\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read numa node type\n");
                usage(argv[0]);
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
                    printf("invalid the number of requests\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read the number of requests\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-h") == 0)
        {
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else
        {
            printf("invalid option: %s\n", argv[i]);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}
