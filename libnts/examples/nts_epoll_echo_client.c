/*
 * <p>Title: nts_epoll_echo_client.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2020 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2020 
 * @version 1.0
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define SERVER_IP "10.10.88.210"
#define DEFAULT_PORT 9922
#define BACKLOG_SIZE 8
#define EPOLL_SIZE 8
#define BUF_LEN 1024

#define MSG "Hello, NTSocks!"

int make_socket_non_blocking(int sockfd)
{
    int flags, s;
    // get current flags
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("get sockfd status");
        return -1;
    }

    flags |= O_NONBLOCK;

    // set flags for sockfd
    s = fcntl(sockfd, F_SETFL, flags);
    if (s == -1)
    {
        perror("set sockfd status");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        fprintf(stdout, "Usage:\n");
        fprintf(stdout, "  %s <host> <port>    \
connect to echo server at <host>:<port>\n",
                argv[0]);
        fprintf(stdout, "\n");
        exit(EXIT_FAILURE);
    }

    int client_fd;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int rc;
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    int server_port = atoi(argv[2]);
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    rc = connect(client_fd,
                 (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rc < 0)
    {
        perror("connect");
        return 2;
    }

    rc = make_socket_non_blocking(client_fd);
    if (rc == -1)
    {
        perror("make_socket_non_blocking");
        return 3;
    }

    // epoll enable
    int epoll_fd = epoll_create(EPOLL_SIZE);
    if (epoll_fd < 0)
    {
        perror("epoll_create");
        return 4;
    }

    struct epoll_event ev, events[EPOLL_SIZE];
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = client_fd;

    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    if (rc < 0)
    {
        perror("epoll_ctl");
        return 5;
    }

    size_t sent_bytes;
    sent_bytes = write(client_fd, MSG, strlen(MSG));
    if (sent_bytes <= 0)
    {
        perror("write");
        return 6;
    }

    for (;;)
    {
        int wait_cnt;
        wait_cnt = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);
        if (wait_cnt == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < wait_cnt; i++)
        {
            if ((events[i].events & EPOLLIN) &&
                events[i].data.fd == client_fd)
            {

                char buf[BUF_LEN] = {0};
                int recv_bytes;
                recv_bytes = read(client_fd, buf, BUF_LEN);
                if (recv_bytes > 0)
                {
                    sent_bytes = write(client_fd, MSG, strlen(MSG));
                    if (sent_bytes <= 0)
                    {
                        perror("write");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    perror("read");
                    close(client_fd);

                    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev);

                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    close(client_fd);

    return 0;
}
