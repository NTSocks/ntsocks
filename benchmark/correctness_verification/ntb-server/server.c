
#include <arpa/inet.h>
#include "../common/common.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
size_t num_req = 1;
size_t payload_size = 16384;

// measure latency with reading ack after writing message
static void latency_write_with_ack(int sockfd)
{
    char msg[payload_size + 1];
    char ack[payload_size + 1];
    int n = 0;
    int ret;

    memset(msg, 0, payload_size + 1);
    memset(ack, 0, payload_size + 1);
    for (size_t i = 0; i < payload_size; i++)
    {
        int j = i % 10;
        msg[i] = (char)('0' + j);
    }

    usleep(50);

    for (size_t i = 0; i < num_req; ++i)
    {
        ret = write(sockfd, msg, payload_size);
        if (ret <= 0)
            printf("[sockid = %d] ret = %d WRITE FAILED!!!!!!!!!!\n", sockfd, ret);

        n = 0;
        while (n != payload_size)
        {
            ret = read(sockfd, ack + n, payload_size);
            if (ret > 0)
            {
                n += ret;
            }
        }

        printf("msg = '%s' \nvs. \n ack = '%s' \n strlen(msg) = %ld , strlen(ack) = %ld\n", 
            msg, ack, strlen(msg), strlen(ack));
        if (strcmp(msg, ack) == 0)
        {
            printf("[verify] true\n");
        }
        else
        {
            printf("[verify] false\n");
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("%s <server ip> <port> \n\n", argv[0]);
        return -1;
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);

    int listen_fd, sockfd;
    struct sockaddr_in address, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

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

    sockfd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);

    latency_write_with_ack(sockfd);

    printf("server done!\n");
    char recv_msg[32] = {0};
    read(sockfd, recv_msg, sizeof(recv_msg));
    close(sockfd);
    close(listen_fd);

    return 0;
}
