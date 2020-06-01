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

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
bool with_ack = false;
int thrds = 0;
int num_req = NUM_REQ;
int run_latency = 0; // by default, we run the throughput benchmark
int payload_size = DEFAULT_PAYLOAD_SIZE;

void usage(char *program);
void parse_args(int argc, char *argv[]);
void *handle_connection(void* ptr);
int connect_setup();
void latency_read(int sockfd);
void latency_read_with_ack(int sockfd);
void pin_1thread_to_1core();

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
    // if(thrds > 0){
    //     free(serv_thread);
    // }
    printf("client done!\n");
    getchar();
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
    for (j = 0; j < thrds; j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       fprintf(stderr,"pthread_setaffinity_np:%d", s);
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
       fprintf(stderr,"pthread_getaffinity_np:%d",s);
    printf("Set returned by pthread_getaffinity_np() contained:\n");
    for (j = 0; j < __CPU_SETSIZE; j++)
        if (CPU_ISSET(j, &cpuset))
            printf("    CPU %d\n", j);
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
    if(with_ack){
        latency_read_with_ack(sockfd);
    }else {
        latency_read(sockfd);
    }
    printf("no close sockfd:%d\n", sockfd);
    // close(sockfd);
    getchar();
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

// write ack after reading message
void latency_read_with_ack(int sockfd){
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

// void throughput_read(int sockfd){
//     latency_read(sockfd);
// }

// void throughput_read_with_ack(int sockfd){

// }

void usage(char *program){
    printf("Usage: \n");
    printf("%s\tstart a server and wait for connection\n", program);
    printf("Options:\n");
    printf(" -p <port>      listen on port number(default %d)\n", DEFAULT_PORT);
    printf(" -t <threads>   handle connection with multi-threads\n");
    printf(" -s <size>      payload size(default %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf(" -n <requests>  the number of request(default %d)\n", NUM_REQ);
    printf(" -w             transfer data with ack\n");
    printf(" -l             run the lantency benchmark\n");
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
            run_latency = 1;
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