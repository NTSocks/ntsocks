#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <math.h>
#include <assert.h>
#include "../common/epoll.h"
#include "../common/common.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
bool with_ack = false;
bool with_echo = true;
bool run_throughput = false;
int conn_num = CONNECT_NUMBER; //connect number default
int num_req = NUM_REQ;
int run_latency = 0; // by default, we run the throughput benchmark
int payload_size = DEFAULT_PAYLOAD_SIZE;
struct CON_INFO *fd_con_info = NULL;
size_t run_num = 0;

void usage(char *program);
void parse_args(int argc, char *argv[]);
void latency_report_perf(size_t *start_cycles, size_t *end_cycles);
void throughput_report_perf(size_t duration);
void latency_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles, char *buf);
bool bandwidth_write(int epfd, int sockfd, char *buf);
void throughput_write_with_ack(int epfd, int sockfd, char *buf);
int connect_setup();
static void
handle_epoll_events(int epfd, struct epoll_event *wait_events, int ret, char *buf);
static void handle_send(int epfd, int socket, char *buf);
static void handle_recv(int epfd, int sockfd, char *buf);
int do_send(int sockfd, char *buf, int send_cnt);
int do_read(int epfd, int sockfd, char *buf);
void close_program(int epfd, int sockfd, char *buf);

void usage(char *program)
{
    printf("Usage: \n");
    printf("%s\tready to connect %s:%d\n", program, server_ip, server_port);
    printf("Options:\n");
    printf(" -a <addr>      connect to server addr(default %s)\n", DEFAULT_SERVER_ADDR);
    printf(" -p <port>      connect to port number(default %d)\n", DEFAULT_PORT);
    printf(" -c <connection number>   connection size \n");
    printf(" -s <size>      payload size(default %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf(" -n <requests>  the number of request(default %d)\n", NUM_REQ);
    printf(" -w             transfer data with ack\n");
    printf(" -l             run the lantency benchmark\n");
    printf(" -t             run the throughput benchmark\n");
    printf(" -e             TURN OFF with echo!!! Please use in latency write!!!\n");
    printf(" -h             display the help information\n");
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
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-e") == 0)
        {
            with_echo = false;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-c") == 0)
        {
            if (i + 1 < argc)
            {
                conn_num = atoi(argv[i + 1]);
                if (conn_num <= 0)
                {
                    printf("invalid numbers of connection\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read numbers of connection\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-l") == 0)
        {
            run_latency = 1;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-t") == 0)
        {
            run_throughput = true;
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

int main(int argc, char *argv[])
{
    // read command line arguments
    setvbuf(stdout, 0, _IONBF, 0);
    parse_args(argc, argv);
    fd_con_info = init_conn_info(conn_num + 100); // initialize connection infomation

    struct epoll_event *wait_event = NULL;
    char *buf = (char *)malloc(payload_size);
    int *socket_set = (int *)malloc(sizeof(int) * conn_num);
    if (socket_set == NULL)
    {
        perror("malloc socket set failed\n");
        exit(EXIT_FAILURE);
    }
    memset(socket_set, 0, sizeof(int) * conn_num);
    int epfd;

    wait_event = (struct epoll_event *)malloc(EPOLLEVENTS * sizeof(struct epoll_event));
    if (wait_event == NULL)
    {
        perror("malloc wait_event memory failed\n");
        exit(EXIT_FAILURE);
    }
    epfd = epoll_create(FDSIZE);
    if (epfd < 0)
    {
        perror("epoll_create failed");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < conn_num; ++i)
    {
        int socket_fd;
        if ((socket_fd = connect_setup(server_port)) < 0)
        {
            perror("connect failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            fd_con_info[socket_fd].recv_cnt = 0;
            fd_con_info[socket_fd].send_cnt = 0;
            set_nonblock(socket_fd);
            epoll_add_event(epfd, socket_fd, EPOLLIN);
            socket_set[i] = socket_fd;
        }
        sleep(1);
    }
    printf("waiting 2 seconds, and then start sending data\n"); 
    sleep(2);

    for (int i = 0; i < conn_num; i++)
    {
        do_send(socket_set[i], buf, payload_size);
    }

    while (true)
    {
        int ret;
        ret = epoll_wait(epfd, wait_event, EPOLLEVENTS, -1);
        if (ret < 0)
        {
            perror("epoll_wait()");
        }
        handle_epoll_events(epfd, wait_event, ret, buf);
    }
    close(epfd);
    return 0;
}

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
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    printf("connecting to %s:%d\n", server_ip, port);
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

static void
handle_epoll_events(int epfd, struct epoll_event *wait_events, int ret, char *buf)
{
    int fd;
    //puts("handle_epoll_events\n");
    for (int i = 0; i < ret; ++i)
    {
        //puts("handle_epoll_events 2\n");
        fd = wait_events[i].data.fd;
        if (wait_events[i].events & EPOLLIN)
        {
            handle_recv(epfd, fd, buf);
            //puts("handle recv");
        }
        else if (wait_events[i].events & (EPOLLERR | EPOLLHUP))
        {
            perror("wait_event: one fd error\n");
            epoll_delete_event(epfd, fd, EPOLLIN);
            fd_con_info[fd].recv_cnt = fd_con_info[fd].send_cnt = 0;
            //close(fd);
        }
        //puts("return handle epoll event\n");
    }
}

// print latency performance data
void latency_report_perf(size_t *start_cycles, size_t *end_cycles)
{
    double *lat = (double *)malloc(num_req * sizeof(double));
    memset(lat, 0, num_req * sizeof(double));
    double cpu_mhz = get_cpu_mhz();
    double sum = 0.0;
    // printf("\nbefore sort:\n");
    for (size_t i = 0; i < num_req; i++)
    {
        lat[i] = (double)(end_cycles[i] - start_cycles[i]) / cpu_mhz;
        sum += lat[i];
    }
    qsort(lat, num_req, sizeof(lat[0]), cmp);
    size_t idx_m, idx_99, idx_99_9, idx_99_99;
    idx_m = floor(num_req * 0.5);
    idx_99 = floor(num_req * 0.99);
    idx_99_9 = floor(num_req * 0.999);
    idx_99_99 = floor(num_req * 0.9999);
    //printf("idx_99 = %lu; idx_99_t = %lu; idx_99_99 = %lu\n", idx_99, idx_99_9, idx_99_99);
    printf("@MEASUREMENT(requests = %d, payload size = %d):\n", num_req, payload_size);
    printf("MEDIAN = %.2f us\n50 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n99.99 TAIL = %.2f us\n", sum / num_req, lat[idx_m], lat[idx_99], lat[idx_99_9], lat[idx_99_99]);
    free(lat);
}

static void handle_recv(int epfd, int sockfd, char *buf)
{
    //puts("hadnle recv and");
    if (!do_read(epfd, sockfd, buf))
    {
        puts("resv data error\n");
        return;
    }
    handle_send(epfd, sockfd, buf);
    //puts("return handle send\n");
    return;
}

static void handle_send(int epfd, int sockfd, char *buf)
{
    // Determine which function to run based on the parameters
    //puts("enter handle_send\n");
    if (run_latency)
    {
        size_t *start_cycles, *end_cycles;
        start_cycles = (size_t *)malloc(sizeof(size_t) * num_req);
        end_cycles = (size_t *)malloc(sizeof(size_t) * num_req);
        memset(start_cycles, 0, sizeof(size_t) * num_req);
        memset(end_cycles, 0, sizeof(size_t) * num_req);
        epoll_delete_event(epfd, sockfd, EPOLLIN);
        latency_write_with_ack(sockfd, start_cycles, end_cycles, buf);
        latency_report_perf(start_cycles, end_cycles);
        free(start_cycles);
        free(end_cycles);
        fd_con_info[sockfd].recv_cnt = fd_con_info[sockfd].send_cnt = 0;
        //epoll_delete_event(epfd, sockfd, EPOLLIN);
        run_num++;
        if (run_num >= conn_num)
        {
            close_program(epfd, sockfd, buf);
        }
        //close(sockfd);
    }
    else
    {
        if (with_ack)
        {
            throughput_write_with_ack(epfd, sockfd, buf);
            //puts("return thoug wite\n");
        }
        else
        {
            bandwidth_write(epfd, sockfd, buf);
            printf("[+]: fd=%d closed\n", sockfd);
            run_num++;
            if (run_num >= conn_num)
            {
                close_program(epfd, sockfd, buf);
            }
            puts("start close\n");
            //close(sockfd);
            //read(sockfd,buf,1);
            puts("close finished\n");
        }
    }
    return;
}

// measure latency with reading ack after writing message
void latency_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles, char *buf)
{                        //
    size_t recv_cnt = 1; //with ack 1
    puts("\nstarting latency \n");
    if (with_echo)
    {
        recv_cnt = payload_size; //recv payload size
    }
    for (size_t i = 0; i < num_req-1; ++i)
    {
        start_cycles[i] = get_cycles();
        size_t n = 0;
        if (!do_send(sockfd, buf, payload_size))
        {
            perror("send data error in latency_write_with_ack\n");
            return;
        }
        printf("[+] fd->%d read %ld \n",sockfd, i);
        int cnt = 0;
        while (n < recv_cnt)
        {
            cnt = 0;
            if ((cnt = read(sockfd, buf, recv_cnt)) < 0)
            {
                //perror("[!!!] send error\n");
                continue;
            }
            else
            {
                n += cnt;
            }
            //printf("%d\n",n);
        }
        end_cycles[i] = get_cycles();
    }
}

// measure throughput without ack after writing message
bool bandwidth_write(int epfd, int sockfd, char *buf)
{
    size_t cnt = 0;
    while (cnt < num_req)
    {
        if (do_send(sockfd, buf, payload_size))
        {
            cnt++;
        }
        else
        {
            perror("send data error\n");
            epoll_delete_event(epfd, sockfd, EPOLLIN);
            fd_con_info[sockfd].recv_cnt = fd_con_info[sockfd].send_cnt = 0;
            return false;
        }
    }
    return true;
}

// measure throughput with reading ack after writing message
void throughput_write_with_ack(int epfd, int sockfd, char *buf)
{
    if (!do_send(sockfd, buf, 1))
    {
        perror("client send data error\n");
        epoll_delete_event(epfd, sockfd, EPOLLIN);
        fd_con_info[sockfd].recv_cnt = fd_con_info[sockfd].send_cnt = 0;
        close(sockfd);
        return;
    }
    fd_con_info[sockfd].send_cnt++;
    //printf("[+] fd-> %d send %ld\n", sockfd,fd_con_info[sockfd].send_cnt);

    if (fd_con_info[sockfd].send_cnt >= num_req)
    {
        printf("fd:%d finshed\n", sockfd);
        
        do_read(epfd, sockfd, buf);
        run_num++;
        epoll_delete_event(epfd,sockfd,EPOLLIN);
        if (run_num >= conn_num)
        {
            puts("close\n");
            close_program(epfd, sockfd, buf);
        }
        puts("start close\n");
        //epoll_delete_event(epfd,sockfd,EPOLLIN);
        //close(sockfd);
        //read(sockfd,buf,1);
        puts("close fininshed\n");
    }
    return;
}

int do_send(int sockfd, char *buf, int send_cnt)
{
    int nwrite = 0;
    while (nwrite < send_cnt)
    {
        int n = 0;
        n = write(sockfd, buf, send_cnt);
        if (n == -1)
        {
            continue;
        }
        nwrite += n;
    }

    return 1;
}

int do_read(int epfd, int sockfd, char *buf)
{
    int n = 0;

    int read_cnt = payload_size;
    if (!with_echo)
    {
        read_cnt = 1;
    }
    //puts("in read\n");
    while (n < read_cnt)
    {
        int t = 0;
        t = read(sockfd, buf, read_cnt);
        if (t == -1)
        {
            // perror("read error:");
            // epoll_delete_event(epfd, sockfd, EPOLLIN);
            // close(sockfd);
            // return 0;
            continue;
        }
        else
            n += t;
    }
    return 1;
}

void close_program(int epfd, int sockfd, char *buf)
{
    int cnt = 0;
    //sleep(5);
    puts("\n[+] ready close program\n");
    while (strncmp(buf, "close", 5))
        while (cnt < 5)
        {
            int n;
            n = read(sockfd, buf, 5);
            if (n >= 0)
            {
                cnt += n;
            }
        }
    cnt = 0;
    puts("[+] already recv close command\n");
    while (cnt <= 5)
        cnt += write(sockfd, "close", 5);
    sleep(1);
    //close(sockfd);
    //close(epfd);
    free(fd_con_info);
    puts("[+] program client finished\n");
    fflush(stdout);
    exit(0);
}
