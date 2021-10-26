#define _GNU_SOURCE
#include "common.h"
#include "client_api.h"
#include "kv_log.h"
#include "measure.h"
#include "batch_api.h"
// DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);

char *ip = DEFAULT_SERVER;
int port = DEFAULT_PORT;
int thrds = 1;
unsigned long max_mempool_size = MAX_MEMPOOL_SIZE;
unsigned long each_chunk_size = EACH_CHUNK_SIZE;
bool waiting_transfer_data = true;
int num_data = 2;

void usage(char *program);
void parse_args(int argc, char *argv[]);
int connect_setup();
void *handle_connection(void *ptr);
void pin_1thread_to_1core();
void test_func1(int sockfd);
void tput(int sockfd);
void batch_tput(int sockfd);
void report_perf(char *cmd, size_t duration, int sockfd);


int main(int argc, char *argv[]){
    parse_args(argc, argv);
    INFO("each chunk size=%lu, max mempool size=%lu, number of put data=%d", each_chunk_size, max_mempool_size, num_data);
    pthread_t serv_thread[MAX_THREADS];
    uint8_t need_lock = thrds > 1 ? 1 : 0;
    mp = mempool_init(each_chunk_size, max_mempool_size, need_lock);
    for (int i = 0; i < thrds; ++i){
        int *socket_fd = mempool_alloc(mp, sizeof(int));
        if((*socket_fd = connect_setup(port)) < 0){
            perror("connect failed");
            exit(EXIT_FAILURE);
        } else if (pthread_create(&serv_thread[i], NULL, handle_connection, socket_fd) < 0){
            perror("Error: create pthread");
        }
    }
    int second = 2;
    INFO("going to sleep %ds", second);
    sleep(second);
    waiting_transfer_data = false;
    
    for(int i=0; i < thrds; ++i) {
        pthread_join(serv_thread[i], NULL);
    }

    printf("client done!\n");
    mempool_clear(mp);
    getchar();
    mempool_destroy(mp);
    return 0;
}

void *handle_connection(void *ptr){
    // INFO("--------------------------test_func1--------------------------");
    // test_func1(sockfd);
    int sockfd = *(int*)ptr;
    mempool_free(mp, ptr);
    pin_1thread_to_1core();
    if (thrds > 1) {
        while (waiting_transfer_data) sched_yield();
        int start = 1;
        write(sockfd, &start, sizeof(int));
    }
    INFO("sockfd:%d start handle command", sockfd);
    INFO("--------------------------test_func2--------------------------");
    // tput(sockfd);
    // map_free(sockfd);
    batch_tput(sockfd);
    map_free(sockfd);
    INFO("map_free");
    map_destroy(sockfd);
    INFO("map_destroy");
    close(sockfd);
}

void batch_tput(int sockfd){
    int i = 0;
    // init item
    char *key_prefix = "batch-key", *value_prefix = "batch-value";
    batch_node_t **item = (batch_node_t**)mempool_alloc(mp, num_data * sizeof(batch_node_t*));
    for (i = 0; i < num_data; ++i){
        item[i] = (batch_node_t*)mempool_alloc(mp, sizeof(batch_node_t));
        memset(item[i], 0, sizeof(batch_node_t));
        void *data = mempool_alloc(mp, 1024);
        snprintf(data, 1024, "%s-%d", key_prefix, i);
        item[i]->key_len = strlen(data) + 1;
        snprintf(data + item[i]->key_len, 1024 - item[i]->key_len, "%s-%d", value_prefix, i);
        item[i]->value_len = strlen(data + item[i]->key_len) + 1;
        item[i]->data = data;
    }
    
    batch_data_t *batch_queue = batch_init(mp, sockfd, BATCH_QUEUE_SIZE);
    batch_reply_t reply;
    size_t start = get_cycles(), end;
    for (i = 0; i < num_data; ++i){
        INFO("batch_push_put, key = %s, value = %s", item[i]->data, item[i]->data + item[i]->key_len);
        int err_no = batch_push(batch_queue, item[i], BATCH_PUT, &reply);
        // INFO("map_put[%d], reply.errno=%s, key=%s, value=%s", i, status_name[reply.err_no], key->data, value->data);
    }
    reply = batch_write_put(batch_queue);
    end = get_cycles();
    printf("report perf(requests = %d, sockfd = %d):\n", num_data, sockfd);
    report_perf("BATCH PUT", end - start, sockfd);

    for (i = 0; i < num_data; ++i)
        item[i]->value_len = 0;
    start = get_cycles();
    for (i = 0; i < num_data; ++i){
        int err_no = batch_push(batch_queue, item[i], BATCH_GET, &reply);
    }
    reply = batch_write_get(batch_queue);
    end = get_cycles();
    report_perf("BATCH GET", end - start, sockfd);
    // free item
    for (i = 0; i < num_data; ++i){
        mempool_free(mp, item[i]->data);
        mempool_free(mp, item[i]);
    }
    mempool_free(mp, item);
    batch_destroy(batch_queue);
    // mempool_free(mp, value);
}

void tput(int sockfd){
    // init key/value
    int i;
    char *key_prefix = "my-key", *value_prefix = "my-value";
    item_t **key, **value;
    key = (item_t**)mempool_alloc(mp, num_data * sizeof(item_t*));
    value = (item_t**)mempool_alloc(mp, num_data * sizeof(item_t*));
    // batch_node_t **item = (batch_node_t**)mempool_alloc(mp, num_data * sizeof(batch_node_t*));
    for (i = 0; i < num_data; ++i){
        // item[i] = (batch_node_t*)mempool_alloc(mp, sizeof(batch_node_t));
        key[i] = (item_t*) mempool_alloc(mp, sizeof(item_t));
        value[i] = (item_t*) mempool_alloc(mp, sizeof(item_t));
        // memset(item[i], 0, sizeof(batch_node_t));
        memset(key[i], 0, sizeof(item_t));
        memset(value[i], 0, sizeof(item_t));
        void *data = mempool_alloc(mp, 1024);
        snprintf(data, 1024, "%s-%d", key_prefix, i);
        key[i]->len = strlen(data) + 1;
        key[i]->data = data;
        snprintf(data + key[i]->len, 1024 - key[i]->len, "%s-%d", value_prefix, i);
        value[i]->len = strlen(data + key[i]->len) + 1;
        value[i]->data = data + key[i]->len;
    }
    reply_t reply;
    size_t start = get_cycles(), end;
    for (i = 0; i < num_data; ++i){
        reply = map_put(sockfd, key[i], value[i]);
        INFO("map_put[%d], reply.errno=%s, key=%s, value=%s", i, status_name[reply.err_no], key[i]->data, value[i]->data);
    }
    end = get_cycles();
    printf("report perf(requests = %d, sockfd = %d):\n", num_data, sockfd);
    report_perf("PUT", end - start, sockfd);

    item_t *v = (item_t*)mempool_alloc(mp, sizeof(item_t));
    memset(v, 0, sizeof(item_t));
    start = get_cycles();
    for (i = 0; i < num_data; ++i){
        reply = map_get(sockfd, key[i], (item_t**)&v);
        if (reply.err_no == MAP_OK){
            INFO("map_get[%d], reply.errno=%s, key=%s, reply.value=%s", i, status_name[reply.err_no], key[i]->data, v->data);
            mempool_free(mp, v->data);
        }else{
            INFO("map_get[%d], reply.errno=%s, key=%s", i, status_name[reply.err_no], key[i]->data);
        }
    }
    end = get_cycles();
    report_perf("GET", end - start, sockfd);
    mempool_free(mp, v);
    for (i = 0; i < num_data; ++i){
        mempool_free(mp, key[i]->data);
        mempool_free(mp, key[i]);
        mempool_free(mp, value[i]);
    }
    mempool_free(mp, key);
    mempool_free(mp, value);
}

void test_func1(int sockfd){
    item_t *key, *value;

    key = (item_t*)mempool_alloc(mp, sizeof(item_t));
    memset(key, 0, sizeof(item_t));

    value = (item_t*)mempool_alloc(mp, sizeof(item_t));
    memset(value, 0, sizeof(item_t));
    key->data = "mykey111";
    key->len = strlen(key->data) + 1;
    value->data = "mydata111";
    value->len = strlen(value->data) + 1;
    
    reply_t reply = map_put(sockfd, key, value);
    INFO("map_put, reply.errno=%s, key=%s, value=%s", status_name[reply.err_no], key->data, value->data);

    item_t *v = (item_t*)mempool_alloc(mp, sizeof(item_t));
    memset(v, 0, sizeof(item_t));
    reply = map_get(sockfd, key, (item_t**)&v);
    if (reply.err_no == MAP_OK){
        INFO("map_get, reply.errno=%s, key=%s, reple.value=%s", status_name[reply.err_no], key->data, v->data);
        // free(v->data);
        mempool_free(mp, v->data);
    }else{
        INFO("map_get, reply.errno=%s, key=%s", status_name[reply.err_no], key->data);
    }
    key->data = "mykey222";
    key->len = strlen(key->data) + 1;
    value->data = "mydata222";
    value->len = strlen(value->data) + 1;

    reply = map_put(sockfd, key, value);
    INFO("map_put, reply.errno=%s, key=%s, value=%s", status_name[reply.err_no], key->data, value->data);
    reply = map_get(sockfd, key, (item_t**)&v);
    if (reply.err_no == MAP_OK){
        INFO("map_get, reply.errno=%s, key=%s, reple.value=%s", status_name[reply.err_no], key->data, v->data);
        // free(v->data);
        mempool_free(mp, v->data);
    }else{
        INFO("map_get, reply.errno=%s, key=%s", status_name[reply.err_no], key->data);
    }
    value->data = "mydata333";
    reply = map_put(sockfd, key, value);
    INFO("map_put, reply.errno=%s, key=%s, value=%s", status_name[reply.err_no], key->data, value->data);

    INFO("map size, reply.len=%lu", map_size(sockfd));
    int err_no = map_remove(sockfd, key);
    INFO("map remove, reply.errno=%s, key=%s", status_name[err_no], key->data);
    
    INFO("map size, reply.len=%lu", map_size(sockfd));
    reply = map_get(sockfd, key, (item_t**)&v);
    if (reply.err_no == MAP_OK){
        INFO("map_get, reply.errno=%s, key=%s, reple.value=%s", status_name[reply.err_no], key->data, v->data);
        // free(v->data);
        mempool_free(mp, v->data);
    }else {
        INFO("map_get, reply.errno=%s, key=%s", status_name[reply.err_no], key->data);
    }
    err_no = map_remove(sockfd, key);
    INFO("map remove, reply.errno=%s, key=%s", status_name[err_no], key->data);

    // free(v);
    mempool_free(mp, v);
    // free(key);
    mempool_free(mp, key);
    // free(value);
    mempool_free(mp, value);
}

void report_perf(char *cmd, size_t duration, int sockfd){
    double cpu_mhz = get_cpu_mhz();
    double total_time = (double)duration / cpu_mhz;
    double tput = (double)num_data / total_time * 1000000;
    printf("%s: %.2f requests per second, total time:%.2f us\n", cmd, tput, total_time);
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
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
        DEBUG("pthread_getaffinity_np:%d",s);
    DEBUG("Set returned by pthread_getaffinity_np() contained:");
    for (j = 0; j < __CPU_SETSIZE; j++)
        if (CPU_ISSET(j, &cpuset))
            DEBUG("    CPU %d", j);
}

int connect_setup(){
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
    if((server = gethostbyname(ip)) == NULL){
        perror("server is NULL");
        exit(EXIT_FAILURE);
    }
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    printf("connected to %s:%d\n", ip, port);
    return socket_fd;
}

void usage(char *program){
    printf("Usage: \n");
    printf("%s\tconnect to %s:%d\n", program, ip, port);
    printf("Options:\n");
    printf(" -s <server>                        connect to server address(default %s)\n", DEFAULT_SERVER);
    printf(" -p <port>                          connect to server port(default %d)\n", DEFAULT_PORT);
    printf(" -t <threads>                       handle connection with multi-threads(default 1, max %d)\n", MAX_THREADS);
    printf(" -m <max-mempool-size(GB/MB/KB)>    maximum memory pool size(default %d)\n", MAX_MEMPOOL_SIZE);
    printf(" -e <each-chunk-size(GB/MB/KB)>     each chunk size(default %d)\n", EACH_CHUNK_SIZE);
    printf(" -n <num-data>                      the number of put data\n");
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
            if(i+1 < argc){
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
        }else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-n") == 0){
            if (i+1 < argc){
                num_data = atoi(argv[i+1]);
                if (num_data <= 0 || num_data > INT32_MAX){
                    printf("invalid number of data\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }else {
                printf("cannot read number of data\n");
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