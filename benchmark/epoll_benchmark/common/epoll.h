#ifndef EPOLL_H_
#define EPOLL_H_

#include <sys/epoll.h>
#include <stdio.h>
#include <string.h>

//connection information
struct CON_INFO
{
    double recv_cnt;
    double send_cnt;
    size_t total_throughput_time; //spend times
    size_t temp_throughput_time; // current time
};

struct CON_INFO *init_conn_info(int size)
{
    struct CON_INFO *conInfo = (struct CON_INFO *)malloc(size * sizeof(struct CON_INFO));
    if (conInfo == NULL)
    {
        perror("malloc CON_INFO failed\n");
        exit(1);
    }
    return conInfo;
}

static void epoll_add_event(int epollfd, int fd, int state);
static void epoll_delete_event(int epollfd, int fd, int state);
static void epoll_modify_event(int epollfd, int fd, int state);

static void epoll_add_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        perror("epoll_add_event error\n");
    }
}

static void epoll_delete_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev) < 0)
    {
        perror("epoll_delete_event error\n");
    }
}

static void epoll_modify_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) < 0)
    {
        perror("epoll_modify_event error\n");
    }
}

void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        perror("fcntl set nonblock failed");
    return;
}

#endif