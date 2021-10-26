#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "server_api.h"
#include "mempool.h"

// DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

char *ip = DEFAULT_SERVER;
int port = DEFAULT_PORT;
int thrds = 1;
unsigned long max_mempool_size = MAX_MEMPOOL_SIZE;
unsigned long each_chunk_size = EACH_CHUNK_SIZE;

void usage(char *program);
void parse_args(int argc, char *argv[]);
int connect_setup();
void *handle_connection(void *ptr);
void pin_1thread_to_1core();

int main(int argc, char *argv[]){
    parse_args(argc, argv);
    INFO("each chunk size=%lu, max mempool size=%lu", each_chunk_size, max_mempool_size);
    int listen_fd = connect_setup();
    pthread_t serv_thread[MAX_THREADS];

    uint8_t need_lock = thrds > 1 ? 1 : 0;
    mp = mempool_init(each_chunk_size, max_mempool_size, need_lock);
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    for (int i = 0; i < thrds; ++i){
        INFO("thread:%d, waiting for accept", i);
        int *sockfd = mempool_alloc(mp, sizeof(int));
        if((*sockfd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len)) < 0){
            perror("accept failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        } else if (pthread_create(&serv_thread[i], NULL, handle_connection, sockfd) < 0){
            close(listen_fd);
            perror("create pthread failed");
        }
    }
    for (int i=0; i < thrds; ++i){
        pthread_join(serv_thread[i], NULL);
    }
    INFO("server done!");
    close(listen_fd);
    mempool_clear(mp);
    getchar();
    mempool_destroy(mp);
    return 0;
}

void *handle_connection(void *ptr){
    int sockfd = *(int*)ptr;
    mempool_free(mp, ptr);
    pin_1thread_to_1core();
    INFO("sockfd:%d wait handle command", sockfd);
    if (thrds > 1){
        int start = 0, n = 0;
        while (true){
            n = read(sockfd, &start, sizeof(int));
            if ((n == sizeof(int)) && start == 1)
                break;
        }
    }
    INFO("sockfd:%d start handle command", sockfd);
    map_t map = new_hashmap(0, NULL);
    batch_message_t *bm;
    batch_data_t *batch_queue = batch_init(mp, sockfd, BATCH_QUEUE_SIZE);

    while (1){
        message_t in;
        int n = 0;
        while(true){
            n += read(sockfd, &in+n , sizeof(message_t)-n);
            if (n == sizeof(message_t))
                break;
        }

        INFO("read %d bytes", n);
        int reply_len = sizeof(reply_t);
        int err_no = 1;
        reply_t out;
        switch (in.cmd){
        case CMD_PUT:
            INFO("receive `PUT` command");
            cmd_put(sockfd, map, &in);
            break;
        case CMD_BATCH_PUT:
            INFO("receive `BATCH_PUT` command");
            bm = (batch_message_t*)&in;
            cmd_batch_put(sockfd, map, bm);
            break;
        case CMD_BATCH_GET:
            INFO("receive `BATCH_GET` command");
            bm = (batch_message_t*)&in;
            cmd_batch_get(sockfd, map, batch_queue, bm);
            break;
        case CMD_GET:
            INFO("receive `GET` command");
            cmd_get(sockfd, map, &in);
            break;
        case CMD_REMOVE:
            INFO("receive `REMOVE` command");
            cmd_remove(sockfd, map, &in);
            break;
        case CMD_LEN:
            INFO("receive `LEN` command");
            cmd_len(sockfd, map);
            break;
        case CMD_FREE:
            INFO("receive `FREE` command");
            cmd_free(map);
            break;
        case CMD_DESTROY:
            INFO("receive `DESTROY` command");
            cmd_destroy(map);
            goto ret;
        default:
            //if fail, do this
            out.key_len = 0;
            out.value_len = 0;
            out.err_no = err_no;
            while(1)
            {
                n += write(sockfd, (void*)&out+n, (reply_len-n));
                if(n == reply_len){
                    break;
                }
            }
            break;
        }
    }
    ret:
        close(sockfd);
        batch_destroy(batch_queue);
        return (void*)0;
}

int connect_setup(){
    int listen_fd, sockfd;
    struct sockaddr_in  address;

    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);
    INFO("bind to %s:%d", ip, port);
    if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address))){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if(listen(listen_fd, SOCK_BACKLOG_CONN) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    INFO("server listens on 0.0.0.0:%d", port);
    return listen_fd;
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
        DEBUG("pthread_setaffinity_np:%d", s);
    // s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    // if (s != 0)
    //     DEBUG("pthread_getaffinity_np:%d",s);
    DEBUG("Set returned by pthread_getaffinity_np() contained:");
    for (j = 0; j < __CPU_SETSIZE; j++)
        if (CPU_ISSET(j, &cpuset))
            DEBUG("    CPU %d", j);
}

void usage(char *program){
    printf("Usage: \n");
    printf("%s\tstart server 0.0.0.0:%d\n", program, port);
    printf("Options:\n");
    printf(" -s <server>                        bind to server address(default %s)\n", DEFAULT_SERVER);
    printf(" -p <port>                          bind to server port(default %d)\n", DEFAULT_PORT);
    printf(" -t <threads>                       handle connection with multi-threads(default 1, max %d)\n", MAX_THREADS);
    printf(" -m <max-mempool-size(GB/MB/KB)>    maximum memory pool size(default %d)\n", MAX_MEMPOOL_SIZE);
    printf(" -e <each-chunk-size(GB/MB/KB)>     each chunk size(default %d)\n", EACH_CHUNK_SIZE);
    printf(" -h                                 display the help information\n");
}

void parse_args(int argc, char *argv[]){
    for (int i = 1; i < argc; ++i){
        if (strlen(argv[i]) == 2 && strcmp(argv[i], "-p") == 0){
            if (i+1 < argc){
                port = atoi(argv[i+1]);
                if (port < 0 || port > 65535){
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
            if (i+1 < argc){
                ip = argv[i+1];
                // TODO: check the validation of th ip address
                i++;
            }else {
                printf("cannot read ip address\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-t") == 0){
            if(i+1 < argc){
                thrds = atoi(argv[i+1]);
                if(thrds <= 0 || thrds > MAX_THREADS){
                    fprintf(stdout, "invalid numbers of thread\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                fprintf(stdout, "cannot read numbers of thread\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-m") == 0){
            if (i+1 < argc){
                char str[128], *p;
                max_mempool_size = strtoul(argv[i+1], &p, 10);
                memcpy(str, p, strlen(p) + 1);
                trim(str);
                strupr(str);
                if (strcmp(str, "GB") == 0){
                    max_mempool_size *= GB;
                }else if (strcmp(str, "MB") == 0){
                    max_mempool_size *= MB;
                }else if (strcmp(str, "KB") == 0){
                    max_mempool_size *= KB;
                }
                if (max_mempool_size <=0 || max_mempool_size > 64 * GB){
                    printf("invalid max mempool size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read max mempool size\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-e") == 0){
            if (i+1 < argc){
                char str[128], *p;
                each_chunk_size = strtoul(argv[i+1], &p, 10);
                memcpy(str, p, strlen(p) + 1);
                trim(str);
                strupr(str);
                if (strcmp(str, "GB") == 0){
                    each_chunk_size *= GB;
                }else if (strcmp(str, "MB") == 0){
                    each_chunk_size *= MB;
                }else if (strcmp(str, "KB") == 0){
                    each_chunk_size *= KB;
                }
                if (each_chunk_size <=0 || each_chunk_size > 64 * GB){
                    printf("invalid each chunk size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read each chunk size\n");
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