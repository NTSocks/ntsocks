
#include "../common/common.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
bool with_ack = false;
int thrds = 1;
int num_req = NUM_REQ;
int run_latency = 0; // 0 - lat, 1 - tput, 2 - bw
int payload_size = DEFAULT_PAYLOAD_SIZE;
int partition = 1;
int packet_size = 128;
int time_in_seconds = 5;
int numa_node_type = 2;
int monitoring = 1;
double read_persecond[MAX_NUM_THREADS] = {0};
int running = 1;
bool to_file = false;
char file_path[256];
char file_name[128];

int cmp(const void *a, const void *b)
{
    return *(double *)a > *(double *)b ? 1 : -1;
}

void usage(char *program);
void parse_args(int argc, char *argv[]);
void *handle_connection(void *ptr);
int connect_setup();
void latency_read(int sockfd);
void latency_read_with_ack(int sockfd);
void throughput_read(int sockfd);
void bandwidth_write(int sockfd);
void throughput_write_with_ack(struct conn_ctx *ctx);
void *monitor_throughput(void *ptr);
void throughput_write_with_ack_in_time(struct conn_ctx *ctx);

int main(int argc, char *argv[])
{
    pthread_t serv_thread[MAX_NUM_THREADS];
    struct conn_ctx conns[MAX_NUM_THREADS];
    pthread_t monitor_thead;
    int last_core = 0;
    long cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);

    // read command line arguments
    parse_args(argc, argv);

    /** 
     * numa_node_type is 0: get cpu core from 0,2,4,6,8.....
     * numa_node_type is 1:  get core from 16-31,48-63
     * numa_node_type is 2:  get core from 0-15,32-47
     */
    if (numa_node_type == 0)
    {
        // The number of cores of the machine is always even， eg, 24,64
        last_core = cpu_cores - 2;
    }
    else if (numa_node_type == 1)
    {
        last_core = cpu_cores - 1;
    }
    else if (numa_node_type == 2)
    {
        last_core = 48;
    }

    if (to_file)
    {
        sprintf(file_name, "benchmark_%d_%d_%d_%d_%d_%d.txt",
                run_latency, payload_size, num_req, thrds, partition, packet_size);
        strcat(file_path, file_name);
    }

    // serv_thread = (pthread_t*)malloc(sizeof(pthread_t) * thrds);
    for (int i = 0; i < thrds; ++i)
    {
        int socket_fd = connect_setup(server_port);

        conns[i].id = i;
        conns[i].sockfd = socket_fd;
        if (numa_node_type == 0)
        {
            conns[i].cpumask = get_core_id(&last_core, cpu_cores, 1);
        }
        else if (numa_node_type == 1)
        {
            conns[i].cpumask = get_core_id2(&last_core, 1);
        }
        else if (numa_node_type == 2)
        {
            conns[i].cpumask = get_core_id3(&last_core, 1);
        }

        if (socket_fd < 0)
        {
            perror("connect failed");
            exit(EXIT_FAILURE);
        }
        else if (pthread_create(&serv_thread[i],
                                NULL, handle_connection, (void *)&conns[i]) < 0)
        {
            perror("Error: create pthread");
        }
        usleep(1000);
    }

    // When test in throughput case, create a thread to
    //  monitor the amount of requests transferred per second
    if (run_latency == 2 || run_latency == 3)
    {
        pthread_create(&monitor_thead, NULL,
                       monitor_throughput, (void *)&run_latency);
    }

    for (int i = 0; i < thrds; ++i)
    {
        pthread_join(serv_thread[i], NULL);
    }

    if (run_latency == 2 || run_latency == 3)
    {
        pthread_join(monitor_thead, NULL);
    }

    printf("client done!\n");
    return 0;
}

// connect to server:port
int connect_setup(int port)
{
    int socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    // bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if ((server = gethostbyname(server_ip)) == NULL)
    {
        perror("server is NULL");
        exit(EXIT_FAILURE);
    }

    bcopy((char *)server->h_addr,
          (char *)&server_addr.sin_addr.s_addr, server->h_length);
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

void *handle_connection(void *ptr)
{
    struct conn_ctx *ctx;
    ctx = (struct conn_ctx *)ptr;
    int sockfd = ctx->sockfd;

    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    printf("waiting for transfer data on %d, time is %ld.%ld\n",
           sockfd, curr_time.tv_sec, curr_time.tv_usec);

    pin_1thread_to_1core(ctx->cpumask);
    gettimeofday(&curr_time, NULL);
    printf("start transfer data, time is %ld.%ld\n",
           curr_time.tv_sec, curr_time.tv_usec);
    if (run_latency == 0)
    {
        if (with_ack)
        {
            latency_read_with_ack(sockfd);
        }
        else
        {
            latency_read(sockfd);
        }
    }
    else if (run_latency == 1)
    {
        bandwidth_write(sockfd);
    }
    else if (run_latency == 2)
    {
        throughput_write_with_ack(ctx);
    }
    else if (run_latency == 3)
    {
        throughput_write_with_ack_in_time(ctx);
    }
    // close(sockfd);
    return (void *)0;
}

void *monitor_throughput(void *ptr)
{
    // test throughput in request num limit or time limit;
    int *ways = (int *)ptr;
    // requests transferred per second per thread
    double throughput[MAX_NUM_THREADS][MAX_TIME_IN_BANDWIDTH];
    int seconds = 0;
    int real = 0;                       // The number of non-zero elements in the throughput array
    double *totalthroughput_eachsecond; // requests transmitted by all threads in every second
    double allthroughput = 0;           // All requests transferred
    time_t tmpcal_ptr;
    FILE *fp;
    tmpcal_ptr = time(NULL) + time_in_seconds;
    printf("begin measure throughput\n");
    if (*ways == 2) // test throughput in request num limit;
    {
        while (monitoring)
        {
            // Record requests transmitted by each thread in one second separately.
            for (int threadId = 0; threadId < thrds; threadId++)
            {
                throughput[threadId][seconds] = 1.0 * read_persecond[threadId];
                if (DEBUG)
                {
                    printf("thread %d throughput is %.0lf reqs/sec \n",
                           threadId, throughput[threadId][seconds]);
                }
                read_persecond[threadId] = 0;
            }
            seconds++;

            sleep(1);
        }
    }
    else if (*ways == 3) //  test throughput in time  limit;
    {
        while (time(NULL) < tmpcal_ptr)
        {
            // Record requests transmitted by each thread in one second separately.
            for (int threadId = 0; threadId < thrds; threadId++)
            {
                throughput[threadId][seconds] = 1.0 * read_persecond[threadId];
                if (DEBUG)
                {
                    printf("thread %d throughput is %.0lf reqs/sec \n",
                           threadId, throughput[threadId][seconds]);
                }
                read_persecond[threadId] = 0;
            }

            seconds++;

            sleep(1);
        }
        running = 0;
    }

    // Count the amount of requests transferred by all threads per second
    totalthroughput_eachsecond = (double *)malloc(seconds * sizeof(double));
    for (int second = 0; second < seconds; second++)
    {
        for (int threadId = 0; threadId < thrds; threadId++)
        {
            totalthroughput_eachsecond[second] += throughput[threadId][second];
        }
    }

    qsort(totalthroughput_eachsecond, seconds, sizeof(double), cmp);
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
                fprintf(fp, "in second %d, throughput is %.0lf reqs/sec\n",
                        i, totalthroughput_eachsecond[i]);
            }
            else
            {
                printf("in second %d, throughput is %.0lf reqs/sec\n",
                       i, totalthroughput_eachsecond[i]);
            }
        }
        if (totalthroughput_eachsecond[i] != 0)
        {
            allthroughput += totalthroughput_eachsecond[i];
            real++;
        }
    }
    if (to_file)
    {

        fprintf(fp, "average throughput is %.0lf reqs/sec\n", allthroughput / real);
        fprintf(fp, "medium throughput is %.0lf reqs/sec\n", totalthroughput_eachsecond[seconds / 2]);
        fprintf(fp, "max throughput is %.0lf reqs/sec\n", totalthroughput_eachsecond[seconds - 1]);
        fclose(fp);
    }
    else
    {
        printf("average throughput is %.0lf reqs/sec\n", allthroughput / real);
        printf("medium throughput is %.0lf reqs/sec\n", totalthroughput_eachsecond[seconds / 2]);
        printf("max throughput is %.0lf reqs/sec\n", totalthroughput_eachsecond[seconds - 1]);
    }
    free(totalthroughput_eachsecond);
    return (void *)0;
}

// read message without ack
void latency_read(int sockfd)
{
    char msg[payload_size];
    int n = 0;
    for (size_t i = 0; i < num_req; ++i)
    {
        n = payload_size;
        while (n > 0)
        {
            n = (n - read(sockfd, msg, n));
        }
    }
}

int Nwrite(int fd, const char *buf, size_t count)
{
    register ssize_t r;
    register size_t nleft = count;

    while (nleft > 0)
    {
        r = write(fd, buf, nleft);
        if (r < 0)
        {
            switch (errno)
            {
            case EINTR:
            case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
            case EWOULDBLOCK:
#endif
                return count - nleft;

            case ENOBUFS:
                return -1;

            default:
                return -2;
            }
        }
        else if (r == 0)
            return -2;
        nleft -= r;
        buf += r;
    }
    return count;
}
void bandwidth_write(int sockfd)
{
    char msg[payload_size];
    int n;
    time_t tmpcal_ptr;
    tmpcal_ptr = time(NULL) + time_in_seconds;
    while (time(NULL) < tmpcal_ptr)
    {
        n = Nwrite(sockfd, msg, payload_size);
        if (n < 0)
        {
            printf("error, n is %d\n", n);
        }
    }
}
void throughput_write_with_ack(struct conn_ctx *ctx)
{
    char msg[payload_size];
    char ack = '1';
    int n;

    // while (time(NULL) < tmpcal_ptr)
    for (size_t i = 0; i < num_req; ++i)
    {
        n = Nwrite(ctx->sockfd, msg, payload_size);
        if (n < 0)
        {
            printf("error, n is %d\n", n);
        }
        read(ctx->sockfd, &ack, sizeof(ack));
        read_persecond[ctx->id] += 1;
    }
    sleep(3);
    monitoring = 0;
}
void throughput_write_with_ack_in_time(struct conn_ctx *ctx)
{
    char msg[payload_size];
    char ack = '1';
    int n;

    // while (time(NULL) < tmpcal_ptr)
    while (running)
    {
        n = Nwrite(ctx->sockfd, msg, payload_size);
        if (n < 0)
        {
            printf("error, n is %d\n", n);
        }
        read(ctx->sockfd, &ack, sizeof(ack));
        read_persecond[ctx->id] += 1;
    }
}

// write ack after reading message
void latency_read_with_ack(int sockfd)
{
    char msg[payload_size];
    char ack[payload_size];
    int n = 0;
    int ret;
    for (size_t i = 0; i < num_req; ++i)
    {
        n = payload_size;
        while (n > 0)
        {
            n = (n - read(sockfd, msg, n));
        }
        //printf("msg = %s\n", msg);
        memcpy(ack, msg, payload_size);
        ret = write(sockfd, ack, payload_size);
        if (ret != payload_size)
        {
            printf("[sockid = %d] send back %ld ACK msg failed\n", sockfd, i + 1);
        }
    }
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
    printf(" -l <metric>    0 - lat, 1 - bw, 2 - tput\n");
    printf(" -f <filepath>      the file path to save the data\n");
    printf(" -r <partition>     partition\n");
    printf(" -c <packetsize>    packet size\n");
    printf(" -w             transfer data with ack\n");
    printf(" -l <metric>    0 - lat, 1 - bw, 2 - tput in req_num limit, 3 - tput in time limit\n");
    printf(" -h             display the help information\n");
    printf(" -d <duration>        time to measure bandwidth\n");
    printf(" -m <numa node type>       Set the way to bind core,0 \
- get core from 0,2,4,6,8.... 1 - get core from 16-31,48-63. \
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
