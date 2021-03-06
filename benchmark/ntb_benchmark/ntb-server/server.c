#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sched.h>
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
int numa_start = 32, numa_end = 48;

bool to_file = false;
int partition = 1;
int packet_size = 128;
char file_path[256];
char file_name[128];

pthread_mutex_t mutex;

static int cmp(const void *a , const void *b){
    return *(double*)a > *(double*)b ? 1 : -1;
}

typedef struct conn_ctx {
    int sockfd;
    int id;
    int cpumask;
} conn_ctx;

double tput_qps_total[10000];
double tput_bw_total[10000];


static void usage(char *program);
static void parse_args(int argc, char *argv[]);
static void *handle_connection(void* ptr);
static void latency_report_perf(size_t *start_cycles, size_t *end_cycles, int sockfd);
static void latency_report_perf_to_file(size_t *start_cycles, size_t *end_cycles, int sockfd);
static void throughput_report_perf(size_t duration, int sockfd, struct conn_ctx* ctx);
static void latency_write(int sockfd, size_t *start_cycles, size_t *end_cycles);
static void latency_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles);
static void throughput_write(int sockfd, size_t *start_cycles, size_t *end_cycles);
static void throughput_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles);
static void pin_1thread_to_1core();
static void bandwidth_write(int sockfd);

int main(int argc, char *argv[]){
    setvbuf(stdout, 0, _IONBF, 0);

    int listen_fd, sockfd;
    struct sockaddr_in  address, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    pthread_t serv_thread[64];
    struct conn_ctx conns[64];
    // read command line arguments
    parse_args(argc, argv);

    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons(server_port);
    printf("bind to %s:%d\n", server_ip, server_port);
    if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address))){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if(listen(listen_fd, SOCK_BACKLOG_CONN) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("server listens on 0.0.0.0:%d\n", server_port);

    if (to_file) {
        sprintf(file_name, "NTSocks_benchmark_%d_%d_%d_%d_%d_%d.txt",
                run_latency, payload_size, num_req, thrds, partition, packet_size);
        strcat(file_path, file_name);
    }

    if(thrds > 0){
        for(int i=0; i < thrds; ++i) {
            printf("thread:%d, waiting for accept\n", i);
            sockfd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
            conns[i].id = i;
            conns[i].sockfd = sockfd;
            if(sockfd < 0){
                perror("accept failed");
                close(listen_fd);
                exit(EXIT_FAILURE);
            } else if (pthread_create(&serv_thread[i], NULL, handle_connection, (void*)&conns[i]) < 0){
                close(listen_fd);
                perror("Error: create pthread");
            }
        }
    }else{
        if((sockfd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len)) < 0){
            perror("accept failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }
        conns[0].id = 0;
        conns[0].sockfd = sockfd;
        handle_connection((void*)&conns[0]);
        
    }

    printf("going to sleep 1s\n");
    sleep(1); // wait 1s
    waiting_transfer_data = false;
    for (int i=0; i < thrds; ++i){
        pthread_join(serv_thread[i], NULL);
    }

    if (thrds > 0) {
        double sum_qps_tput = 0;
        double sum_bw_tput = 0;
        for (int i = 0; i < thrds; i++)
        {
            sum_qps_tput += tput_qps_total[i];
            sum_bw_tput += tput_bw_total[i];
        }

        printf("@TOTAL_MEASUREMENT(requests = %d, payload size = %d):\n\
Overall QPS TPUT = %.2f REQ/s\nOverall BandWidth = %.2f Gbps\n", 
        num_req, payload_size, sum_qps_tput, sum_bw_tput);

    }

    printf("server done!\n");
    // getchar();
    // close(listen_fd);
    
    return 0;
}

// each thread pins to one core
void pin_1thread_to_1core(){
    cpu_set_t cpuset;
    pthread_t thread;
    thread = pthread_self();
    CPU_ZERO(&cpuset);
    int j,s;
    for (j = numa_start; j < numa_end; j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       printf("pthread_setaffinity_np:%d", s);
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       printf("pthread_getaffinity_np:%d",s);
    // printf("Set returned by pthread_getaffinity_np() contained:\n");
    // for (j = 0; j < __CPU_SETSIZE; j++)
    //     if (CPU_ISSET(j, &cpuset))
    //         printf("    CPU %d\n", j);
}

void *handle_connection(void* ptr){
    struct conn_ctx * ctx;
    ctx = (struct conn_ctx *) ptr;
    int sockfd = ctx->sockfd;
    printf("waiting for transfer data on %s:%d\n", server_ip, sockfd);
    if(thrds > 0){
        pin_1thread_to_1core();
        // wait for starting transfer data
        while (waiting_transfer_data) sched_yield();
    }
    printf("start transfer data\n");
    // if(thrds > 0){
    //     int start = 1;
    //     write(sockfd, &start, sizeof(int));
    // }

    // Determine which function to run based on the parameters
    if(run_latency == 0){
        size_t *start_cycles, *end_cycles;
        start_cycles = (size_t*)malloc(sizeof(size_t) * num_req);
        end_cycles = (size_t*)malloc(sizeof(size_t) * num_req);
        if(with_ack){
            latency_write_with_ack(sockfd, start_cycles, end_cycles);
        }else {
            latency_write(sockfd, start_cycles, end_cycles);
        }

        if (to_file){
            latency_report_perf_to_file(start_cycles, end_cycles, sockfd);
        } else {
            latency_report_perf(start_cycles, end_cycles, sockfd);
        }

        free(start_cycles);
        free(end_cycles);
    }else if (run_latency == 1){
        size_t start_cycles, end_cycles;
        if(with_ack) {
            throughput_write_with_ack(sockfd, &start_cycles, &end_cycles);
        }else {
            throughput_write(sockfd, &start_cycles, &end_cycles);
        }
        throughput_report_perf(end_cycles - start_cycles, sockfd, ctx);
    }else if (run_latency == 2)
        bandwidth_write(sockfd);
    // char end_msg[32] = {0};
    // read(sockfd, end_msg, sizeof(end_msg));
    fflush(stdout);
    // printf("no close sockfd:%d\n", sockfd);
    // close(sockfd);
    // getchar();
    return (void*)0;
}

// measure latency without ack after writing message
void latency_write(int sockfd, size_t *start_cycles, size_t *end_cycles){
    char msg[payload_size];
    for (size_t i = 0; i < num_req; i++) {
        start_cycles[i] = get_cycles();
        write(sockfd, msg, payload_size);
        end_cycles[i] = get_cycles();
    }
}

// measure latency with reading ack after writing message
void latency_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles){
    char msg[payload_size];
    char ack[payload_size];
    int n = 0;
    int ret;
    for (size_t i = 0; i < num_req; ++i){
        start_cycles[i] = get_cycles();

        // memset(msg, 0, payload_size);
        // sprintf(msg, "%d-%ld", sockfd, i+1);
        ret = write(sockfd, msg, payload_size);
        // printf("[sockid = %d] write ret = %d, seq = %ld, msg='[%s]'\n", sockfd, ret, i+1, msg);
        if (ret <= 0) printf("[sockid = %d] ret = %d WRITE FAILED!!!!!!!!!!\n", sockfd, ret);

        n = payload_size;
        while (n > 0) {
            n = (n - read(sockfd, ack, n));
        }
        end_cycles[i] = get_cycles();
    }
}

// measure throughput without ack after writing message
void throughput_write(int sockfd, size_t *start_cycles, size_t *end_cycles){
    char msg[payload_size];
    *start_cycles = get_cycles();
    for (size_t i = 0; i < num_req; i++) {
        write(sockfd, msg, payload_size);
    }
    *end_cycles = get_cycles();
}

// measure throughput with reading ack after writing message
void throughput_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles){
    char msg[payload_size];
    char ack = '1';
    *start_cycles = get_cycles();
    for (size_t i = 0; i < num_req; ++i){
        write(sockfd, msg, payload_size);
        read(sockfd, &ack, sizeof(ack));
    }
    *end_cycles = get_cycles();
}

void bandwidth_write(int sockfd){
    char msg[payload_size];
    while (1){
        write(sockfd, msg, payload_size);
    }
}

// print latency performance data
void latency_report_perf(size_t *start_cycles, size_t *end_cycles, int sockfd) {
    double* lat = (double*)malloc(num_req * sizeof(double));
    double cpu_mhz = get_cpu_mhz();
    double sum = 0.0;
    // printf("\nbefore sort:\n");
    for (size_t i = 0; i < num_req; i++) {
        lat[i] = (double)(end_cycles[i] - start_cycles[i]) / cpu_mhz;
        sum += lat[i];
    }
    qsort(lat, num_req, sizeof(lat[0]), cmp);
    size_t idx_m, idx_99, idx_99_9, idx_99_99;
    idx_m = floor(num_req * 0.5);
    idx_99 = floor(num_req * 0.99);
    idx_99_9 = floor(num_req * 0.999);
    idx_99_99 = floor(num_req * 0.9999);

    printf("@MEASUREMENT(requests = %d, payload size = %d, sockfd = %d):\n\
MEDIAN = %.2f us\n50 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n99.99 TAIL = %.2f us\n", 
        num_req, payload_size, sockfd, sum/num_req, lat[idx_m], lat[idx_99], lat[idx_99_9], lat[idx_99_99]);
    
    free(lat);
}


void latency_report_perf_to_file(size_t *start_cycles, size_t *end_cycles, int sockfd) {

    double* lat = (double*)malloc(num_req * sizeof(double));
    double cpu_mhz = get_cpu_mhz();
    double sum = 0.0;
    // printf("\nbefore sort:\n");
    for (size_t i = 0; i < num_req; i++) {
        lat[i] = (double)(end_cycles[i] - start_cycles[i]) / cpu_mhz;
        sum += lat[i];
    }
    qsort(lat, num_req, sizeof(lat[0]), cmp);
    size_t idx_m, idx_99, idx_99_9, idx_99_99;
    idx_m = floor(num_req * 0.5);
    idx_99 = floor(num_req * 0.99);
    idx_99_9 = floor(num_req * 0.999);
    idx_99_99 = floor(num_req * 0.9999);

    // Lock
    pthread_mutex_lock(&mutex);

    log_ctx_t ctx = log_init(file_path, strlen(file_path));

    fprintf(ctx->file, "@MEASUREMENT(requests = %d, payload size = %d, sockfd = %d):\n\
MEDIAN = %.2f us\n50 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n99.99 TAIL = %.2f us\n", 
        num_req, payload_size, sockfd, sum/num_req, lat[idx_m], lat[idx_99], lat[idx_99_9], lat[idx_99_99]);
    
    log_destroy(ctx);

    // Unlock
    pthread_mutex_unlock(&mutex);

    free(lat);
}


// print throughput performance data
void throughput_report_perf(size_t duration, int sockfd, struct conn_ctx* ctx) {
    double cpu_mhz = get_cpu_mhz();
    double total_time = (double)duration / cpu_mhz;
    // throughput
    double tput1 = (double)num_req / total_time * 1000000;  // reqs/s
    double tput_bw = tput1 * payload_size * 8 / (1024*1024*1024);
    // double tput_bw = ((double)num_req * payload_size * 8 * 1000000) / ((double)total_time * 1024 * 1024 * 1024);   // Unit: Gbps
    // double tput_bw = num_req * payload_size * 8 / total_time;

    tput_qps_total[ctx->id] = tput1;
    tput_bw_total[ctx->id] = tput_bw;

    // bandwidth
    printf("@MEASUREMENT(requests = %d, payload size = %d, sockfd = %d):\n\
total time = %.2f us\nTHROUGHPUT1 = %.2f REQ/s\nBandWidth = %.2lf Mbps\n", 
        num_req, payload_size, sockfd, total_time, tput1, tput_bw);
}

void usage(char *program){
    printf("Usage: \n");
    printf("%s\tready to connect %s:%d\n", program, server_ip, server_port);
    printf("Options:\n");
    printf(" -a <addr>      connect to server addr(default %s)\n", DEFAULT_SERVER_ADDR);
    printf(" -p <port>      connect to port number(default %d)\n", DEFAULT_PORT);
    printf(" -t <threads>   handle connection with multi-threads\n");
    printf(" -s <size>      payload size(default %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf(" -n <requests>  the number of request(default %d)\n", NUM_REQ);
    printf(" -l <metric>    0 - lat, 1 - tput, 2 - bw\n");
    printf(" -f <filepath>      the file path to save the data\n");
    printf(" -r <partition>     partition\n");
    printf(" -c <packetsize>    packet size\n");
    printf(" -w             transfer data with ack\n");
    printf(" -l             run the lantency benchmark\n");
    printf(" -k             over kernel socket(defaule over ntb)");
    printf(" -h             display the help information\n");
}

// parsing command line parameters
void parse_args(int argc, char *argv[]){
    for (int i = 1; i < argc; ++i){
        if (strlen(argv[i]) == 2 && strcmp(argv[i], "-p") == 0){
            if (i+1 < argc){
                server_port = atoi(argv[i+1]);
                if (server_port < 0 || server_port > 65535){
                    printf("invalid port number\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read port number\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-a") == 0){
            if(i+1 < argc) {
                server_ip = argv[i+1];
                i++;
            }else {
                printf("cannot read server addr\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-s") == 0){
            if(i+1 < argc){
                payload_size = atoi(argv[i+1]);
                if(payload_size <= 0){
                    printf("invalid payload size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read payload size\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-w") == 0){
            with_ack = true;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-f") == 0){
            to_file = true;
            if(i+1 < argc){
                strcpy(file_path, argv[i+1]);
                i++;
            }else {
                printf("cannot read file path\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-k") == 0){
            numa_start = 48;
            numa_end = 64;
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-t") == 0){
            if(i+1 < argc){
                thrds = atoi(argv[i+1]);
                if(thrds <= 0 || thrds > MAX_NUM_THREADS){
                    printf("invalid numbers of thread\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read numbers of thread\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-r") == 0){
            if(i+1 < argc){
                partition = atoi(argv[i+1]);
                if(partition <= 0){
                    printf("invalid partition number\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read partition number\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-c") == 0){
            if(i+1 < argc){
                packet_size = atoi(argv[i+1]);
                if(packet_size <= 0){
                    printf("invalid packet size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read packet size\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-l") == 0){
            if(i+1 < argc){
                run_latency = atoi(argv[i+1]);
                if(run_latency != 0 && run_latency != 1 && run_latency != 2){
                    printf("invalid metric\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read metric\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-n") == 0){
            if(i+1 < argc){
                num_req = atoi(argv[i+1]);
                if(num_req <= 0){
                    printf("invalid the number of requests\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read the number of requests\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-h") == 0){
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        }else {
            printf("invalid option: %s\n", argv[i]);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}
