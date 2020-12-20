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
#include <math.h>
#include <sys/time.h>
#include <sched.h>
#include "../common/common.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
bool with_ack = false;
int thrds = 0;
int num_req = NUM_REQ;
int run_latency = 0; // 0 - lat, 1 - tput, 2 - bw
int payload_size = DEFAULT_PAYLOAD_SIZE;
int numa_start = 32, numa_end = 48;

void usage(char *program);
void parse_args(int argc, char *argv[]);
void *handle_connection(void* ptr);
int connect_setup();
void latency_read(int sockfd);
void latency_read_with_ack(int sockfd);
void pin_1thread_to_1core();
void throughput_read(int sockfd);
void throughput_read_with_ack(int sockfd);
void bandwidth_read(int sockfd);

int main(int argc, char *argv[]){
    pthread_t serv_thread[64];

    // read command line arguments
    parse_args(argc, argv);

    if(thrds > 0) {
        // serv_thread = (pthread_t*)malloc(sizeof(pthread_t) * thrds);
        for(int i=0; i < thrds; ++i) {
            int socket_fd;
            if((socket_fd = connect_setup(server_port)) < 0){
                perror("connect failed");
                exit(EXIT_FAILURE);
            } else if (pthread_create(&serv_thread[i], NULL, handle_connection, (void*)&socket_fd) < 0){
                perror("Error: create pthread");
            }
            usleep(1000);
        }
    }else {
        int socket_fd = connect_setup(server_port);
        handle_connection((void *)&socket_fd);
    }
    for(int i=0; i < thrds; ++i) {
        pthread_join(serv_thread[i], NULL);
    }
  
    printf("client done!\n");
    return 0;
}

// connect to server:port
int connect_setup(int port){
    int socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    // bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if((server = gethostbyname(server_ip)) == NULL){
        perror("server is NULL");
        exit(EXIT_FAILURE);
    }
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
    for (j = numa_start; j < numa_end; j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       fprintf(stderr,"pthread_setaffinity_np:%d", s);
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       fprintf(stderr,"pthread_getaffinity_np:%d",s);
    // printf("Set returned by pthread_getaffinity_np() contained:\n");
    // for (j = 0; j < __CPU_SETSIZE; j++)
    //     if (CPU_ISSET(j, &cpuset))
    //         printf("    CPU %d\n", j);
}

void *handle_connection(void* ptr){
    int sockfd = *(int*)ptr;

    int start = 0;
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    printf("waiting for transfer data on %d, time is %ld.%ld\n", sockfd, curr_time.tv_sec, curr_time.tv_usec);
    if(thrds > 0){
        pin_1thread_to_1core();
    }
    // if(thrds > 0){
    //     while (true){
    //         start = 0;
    //         read(sockfd, &start, sizeof(int));
    //         if(start == 1) break;
    //         else sched_yield();
    //     }
    // }
    gettimeofday(&curr_time, NULL);
    printf("start transfer data, time is %ld.%ld\n", curr_time.tv_sec, curr_time.tv_usec);
    if (run_latency == 0){
        if(with_ack){
            latency_read_with_ack(sockfd);
        } else {
            latency_read(sockfd);
        }
    } else if (run_latency == 1){
        if(with_ack){
            throughput_read_with_ack(sockfd);
        }else {
            throughput_read(sockfd);
        }
    } else if (run_latency == 2)
        bandwidth_read(sockfd);
    // close(sockfd);
    return (void*)0;
}

// read message without ack
void latency_read(int sockfd){
    char msg[payload_size];
    int n = 0;
    for(size_t i = 0; i < num_req; ++i){
        n = payload_size;
        while (n > 0) {
            n = (n - read(sockfd, msg, n));
        }
    }
}

// read message without ack
void throughput_read(int sockfd){
    char msg[payload_size];
    int n = 0;
    for(size_t i = 0; i < num_req; ++i){
        n = payload_size;
        while (n > 0) {
            n = (n - read(sockfd, msg, n));
        }
    }
}

// write ack after reading message
void latency_read_with_ack(int sockfd){
    char msg[payload_size];
    char ack[payload_size];
    int n = 0;
    int ret;
    for (size_t i = 0; i < num_req; ++i) {
        n = payload_size;
        // memset(msg, 0, payload_size);
        // memset(ack, 0, payload_size);
        while (n > 0) {
            n = (n - read(sockfd, msg, n));
        }
        // printf("[sockid = %d] recv %ld request: %s \n", sockfd, i+1, msg);
        // memcpy(ack, msg, payload_size);
        ret = write(sockfd, ack, payload_size);
        if (ret != payload_size) {
            printf("[sockid = %d] send back %ld ACK msg failed\n", sockfd, i+1);
        }
    }
}

// write ack after reading message
void throughput_read_with_ack(int sockfd){
    char msg[payload_size];
    char ack = '1';
    int n = 0;
    for (size_t i = 0; i < num_req; ++i) {
        n = payload_size;
        while (n > 0) {
            n = (n - read(sockfd, msg, n));
        }
        write(sockfd, &ack, sizeof(ack));
    }
}

void bandwidth_read(int sockfd){
    char msg[payload_size];
    while (read(sockfd, msg, payload_size) != 0) break;

    uint64_t counter = 0;
    size_t start_cycles, end_cycles, elapse_cycles = 0;
    double cpu_mhz = get_cpu_mhz() * 1000000;

    while (1){
        ssize_t r = 0;
        start_cycles = get_cycles();
        r = read(sockfd, msg, payload_size);
        end_cycles = get_cycles();
        if (r > 0)
            counter += r;
        else
            fprintf(stderr, "read data err\n");
        elapse_cycles += (end_cycles - start_cycles);
        if ((double)elapse_cycles >= RUN_TIME * cpu_mhz)
            break;
    }
    double elapse_time = (double)elapse_cycles / cpu_mhz;
    printf("send buff = %d, total elapse_time = %.2lf, trans bytes = %.2lf Mb, BW (Gbits/s) = %.2lf \n", payload_size, elapse_time, (double)counter / (1024 * 1024), (double)counter / (elapse_time * 128 * 1024 * 1024));
}

void usage(char *program){
    printf("Usage: \n");
    printf("%s\tstart a server and wait for connection\n", program);
    printf("Options:\n");
    printf(" -p <port>      listen on port number(default %d)\n", DEFAULT_PORT);
    printf(" -t <threads>   handle connection with multi-threads\n");
    printf(" -s <size>      payload size(default %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf(" -n <requests>  the number of request(default %d)\n", NUM_REQ);
    printf(" -l <metric>    0 - lat, 1 - tput, 2 - bw\n");
    printf(" -w             transfer data with ack\n");
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
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-k") == 0){
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