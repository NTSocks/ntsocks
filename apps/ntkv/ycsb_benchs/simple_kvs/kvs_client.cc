#include "kvs_client.h"

namespace ycsbc {

int KVSClient::handle_msg(int cmd){
    int len = context_->obuf_len;
    int n = write(context_->fd, context_->obuf, len);

    if( n != len){
        error_num++;
        return 0;
    }

    reply_t in;
    while (true){
        n = read(context_->fd, &in, REPLY_SIZE);
        if (n == REPLY_SIZE)
            break;
    }
    switch(cmd){
        case CMD_GET:
            // cout << "handle_msg CMD_GET" << endl;
            if (in.err_no == MAP_OK){
                char *data = (char*)malloc(in.value_len);
                n = 0;
                while (true){
                    n += read(context_->fd, data+n , in.value_len-n);
                    if (n == in.value_len){
                        break;
                    }
                }
                free(data);
            }
            else{
                printf("put fail\n");
                error_num++;
                //return -1;
            }
            
            break;
        case CMD_PUT:
          //  printf("err_no %d\n", in.err_no);
            if (in.err_no == MAP_OK){
                break;
            }
            else if (in.err_no == MAP_REPLACE){
                break;
            }
            else
            {
                printf("put fail\n");
                error_num++;
                //return -1;
            }
            break;
        case CMD_REMOVE:
            // cout << "handle_msg CMD_REMOVE" << endl;
            break;
        default:
            
            break;
    }
    free(context_->obuf);
    return 0;
}

kvsContext* KVSClient::connect_kvs(const char *host, int port){
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
    if((server = gethostbyname(host)) == NULL){
        perror("server is NULL");
        exit(EXIT_FAILURE);
    }
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    printf("kvs client connected to %s:%d\n", host, port);
    kvsContext *context = (kvsContext*)malloc(sizeof(kvsContext));
    memset(context, 0, sizeof(kvsContext));
    context->fd = socket_fd;
    return context;
}

}