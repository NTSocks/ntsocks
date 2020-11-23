#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include <sched.h>
#include <math.h>
#include <assert.h>
#include "../common/common.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
bool with_ack = false;
int thrds = 0;
int num_req = NUM_REQ;
bool waiting_transfer_data = true;
int run_latency = 0; // 0 - lat, 1 - tput, 2 - bw
int payload_size = DEFAULT_PAYLOAD_SIZE;

void usage(char *program);
void parse_args(int argc, char *argv[]);
void *handle_connection(void* ptr);
void latency_report_perf(size_t *start_cycles, size_t *end_cycles);
void throughput_report_perf(size_t duration);
void latency_write(int sockfd, size_t *start_cycles, size_t *end_cycles);
void latency_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles);
void throughput_write(int sockfd, size_t *start_cycles, size_t *end_cycles);
void throughput_write_with_ack(int sockfd, size_t *start_cycles, size_t *end_cycles);
void bandwidth_write(int sockfd);
void pin_1thread_to_1core();
int connect_setup();

int main(int argc, char *argv[]){
    pthread_t serv_thread[70];

    // read command line arguments
    parse_args(argc, argv);

    if(thrds > 0) {
        for(int i=0; i < thrds; ++i) {
            int socket_fd;
            printf("connect socket:%d\n", i);
            if((socket_fd = connect_setup(server_port)) < 0){
                perror("connect failed");
                exit(EXIT_FAILURE);
            } else if (pthread_create(&serv_thread[i], NULL, handle_connection, (void*)&socket_fd) < 0){
                perror("Error: create pthread");
            }
            usleep(10000);
        }
    }else {
        int socket_fd = connect_setup(server_port);
        handle_connection((void *)&socket_fd);
    }
    printf("going to sleep 3s\n");
    sleep(5); // wait 1s
    waiting_transfer_data = false;
    printf("waiting_transfer_data is %d\n", waiting_transfer_data);
    for(int i=0; i < thrds; ++i) {
        pthread_join(serv_thread[i], NULL);
    }
    
    return 0;
}

int connect_setup(int port){
    int socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }

    //Bind to a specific network interface (and optionally a specific local port)
    struct sockaddr_in localaddr;
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = inet_addr("10.10.88.215");
    localaddr.sin_port = 0;  // Any local port will do
    bind(socket_fd, (struct sockaddr *)&localaddr, sizeof(localaddr));

    // bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if((server = gethostbyname(server_ip)) == NULL){
        perror("server is NULL");
        exit(EXIT_FAILURE);
    }
    printf("connecting to %s:%d\n", server_ip, port);
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

// each thread pins to one core
void pin_1thread_to_1core(){
    cpu_set_t cpuset;
    pthread_t thread;
    thread = pthread_self();
    CPU_ZERO(&cpuset);
    int j,s;
    for (j = 0; j < thrds; j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       fprintf(stderr,"pthread_setaffinity_np:%d", s);
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       fprintf(stderr,"pthread_setaffinity_np:%d", s);
    // printf("Set returned by pthread_getaffinity_np() contained:\n");
    // for (j = 0; j < __CPU_SETSIZE; j++)
    //     if (CPU_ISSET(j, &cpuset))
    //         printf("    CPU %d\n", j);
}

void *handle_connection(void* ptr){
    int sockfd = *(int*)ptr;
    printf("waiting for transfer data on %s:%d\n", server_ip, sockfd);
    if(thrds > 0){
        pin_1thread_to_1core();
        while (waiting_transfer_data) sched_yield();
    }
    printf("start transfer data\n");
    if(thrds > 0){
        int start = 1;
        write(sockfd, &start, sizeof(int));
    }

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
        latency_report_perf(start_cycles, end_cycles);
        free(start_cycles);
        free(end_cycles);
    }else if (run_latency == 1){
        size_t start_cycles, end_cycles;
        if(with_ack) {
            throughput_write_with_ack(sockfd, &start_cycles, &end_cycles);
        }else {
            throughput_write(sockfd, &start_cycles, &end_cycles);
        }
        throughput_report_perf(end_cycles - start_cycles);
    }else if (run_latency == 2)
        bandwidth_write(sockfd);
    close(sockfd);
    return (void*)0;
}

void bandwidth_write(int sockfd){
    char msg[payload_size];
    while (1){
        write(sockfd, msg, payload_size);
    }
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
    for (size_t i = 0; i < num_req; ++i){
        start_cycles[i] = get_cycles();
        write(sockfd, msg, payload_size);
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

// print latency performance data
void latency_report_perf(size_t *start_cycles, size_t *end_cycles) {
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
    //printf("idx_99 = %lu; idx_99_t = %lu; idx_99_99 = %lu\n", idx_99, idx_99_9, idx_99_99);
    printf("@MEASUREMENT(requests = %d, payload size = %d):\n", num_req, payload_size);
    printf("MEDIAN = %.2f us\n50 TAIL = %.2f us\n99 TAIL = %.2f us\n99.9 TAIL = %.2f us\n99.99 TAIL = %.2f us\n", sum/num_req, lat[idx_m], lat[idx_99], lat[idx_99_9], lat[idx_99_99]);
    free(lat);
}

// print throughput performance data
void throughput_report_perf(size_t duration) {
    double cpu_mhz = get_cpu_mhz();
    double total_time = (double)duration / cpu_mhz;
    // throughtput
    double tput1 = (double)num_req / total_time * 1000000;
    // bandwidth
    printf("@MEASUREMENT(requests = %d, payload size = %d):\n", num_req, payload_size);
    printf("total time = %.2f us\n", total_time);
    printf("THROUGHPUT = %.2f REQ/s\n", tput1);
    // printf("THROUGHPUT2 = %.2f Mb/s\n", tput2 * 8);
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
    printf(" -w             transfer data with ack\n");
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
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-l") == 0){
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
