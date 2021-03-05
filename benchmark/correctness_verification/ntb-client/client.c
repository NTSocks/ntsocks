
#include "../common/common.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
size_t num_req = 1;
size_t payload_size = 16384;

// connect to server:port
static int connect_setup(int port)
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
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

// write ack after reading message
static void latency_read_with_ack(int sockfd)
{
    char msg[payload_size + 1];
    size_t n = 0;
    int ret;
    for (size_t i = 0; i < num_req; ++i)
    {
        n = 0;
        while (n != payload_size)
        {
            ret = read(sockfd, msg + n, payload_size);
            if (ret > 0)
            {
                n += ret;
            }
        }

        ret = write(sockfd, msg, payload_size);
        if (ret != payload_size)
        {
            printf("[sockid = %d] send back %ld ACK msg failed\n", sockfd, i + 1);
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

    int socket_fd = connect_setup(server_port);

    latency_read_with_ack(socket_fd);

    printf("client done!\n");
    close(socket_fd);

    return 0;
}
