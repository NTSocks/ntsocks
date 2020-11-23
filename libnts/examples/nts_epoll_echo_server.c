/*
 * <p>Title: nts_epoll_echo_server.c</p>
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
#define PORT 9922
#define BACKLOG_SIZE 8
#define EPOLL_SIZE 8
#define BUF_LEN 1024

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
    if (argc != 3) {
        fprintf(stdout, "Usage:\n");
        fprintf(stdout, "  %s <host> <port>    connect to echo server at <host>:<port>\n", argv[0]);
        fprintf(stdout, "\n");
        exit(EXIT_FAILURE);
    }

    int listen_fd;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        return -1;
    }

    int rc;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    int server_port = atoi(argv[2]);
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    rc = bind(listen_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (rc < 0)
    {
        perror("bind");
        return -1;
    }

    /** set listen listen_fd as NON_BLOCKING */
    rc = make_socket_non_blocking(listen_fd);
    if (rc < 0)
    {
        perror("make_socket_non_blocking listen_fd");
        return -1;
    }

    rc = listen(listen_fd, BACKLOG_SIZE);
    if (rc < 0)
    {
        perror("listen");
        return -1;
    }

    // create epoll instance
    int epoll_fd = epoll_create(EPOLL_SIZE);
    printf("epoll_fd: %d\n", epoll_fd);
    if (epoll_fd < 0)
    {
        perror("epoll_create failed");
        return -1;
    }

    struct epoll_event ev, events[EPOLL_SIZE];

    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;
    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
    if (rc == -1)
    {
        perror("epoll_ctl EPOLL_CTL_ADD");
        return -1;
    }

    for (;;)
    {
        int wait_count;
        wait_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);
        if (wait_count == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < wait_count; i++)
        {
            uint32_t ep_events = events[i].events;

            if (ep_events & EPOLLERR || ep_events & EPOLLHUP || (!ep_events & EPOLLIN))
            {
                printf("Epoll has error \n");
                close(events[i].data.fd);
                continue;
            }
            else if (listen_fd == events[i].data.fd)
            { // POLLIN event for listen socket
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(struct sockaddr_in));
                socklen_t client_addrlen = sizeof(client_addr);

                int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
                if (client_fd <= 0)
                    continue;

                rc = make_socket_non_blocking(client_fd);
                if (rc == -1)
                {
                    perror("make_socket_non_blocking");
                    return -1;
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = (uint32_t)client_fd;
                rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                if (rc == -1)
                {
                    perror("epoll_ctl EPOLL_CTL_ADD client_sockfd");
                    return -1;
                }

                continue;
            }
            else
            { // POLLIN event for client socket
                int done = 0;

                int client_fd = (int)events[i].data.fd;

                char buf[BUF_LEN] = {0};
                int bytes_cnt;
                bytes_cnt = read(client_fd, buf, BUF_LEN);
                if (bytes_cnt < 0)
                {
                    close(client_fd);

                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev);
                }
                else if (bytes_cnt == 0)
                {
                    printf(" disconnect %d\n", client_fd);

                    close(client_fd);

                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev);
                    break;
                }
                else
                {
                    printf("Recv: %s, %d Bytes\n", buf, bytes_cnt);
                    write(client_fd, buf, bytes_cnt);
                }
            }
        }
    }

    close(listen_fd);

    return 0;
}
