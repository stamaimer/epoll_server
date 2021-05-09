#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXEVENT 4096

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: tcp_server <ip> <port>\n");
        return -1;
    }

    fprintf(stdout, "server on %s:%d\n", argv[1], atoi(argv[2]));

    int listenfd, acceptfd, epollfd, on = 1, i;
    socklen_t client_addr_size;
    struct sockaddr_in server_addr, client_addr;
    char buf[1024];
    ssize_t num;
    struct epoll_event event;
    struct epoll_event *events;

    // 1. init server_addr
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], &server_addr.sin_addr) == 0)
    {
        perror("inet_aton");
        return -2;
    }

    // 2. create socket
    if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
    {
        perror("socket");
        return -3;
    }

    // 3. SO_REUSEADDR
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
    {
        perror("setsockopt");
        close(listenfd);
        return -4;
    }

    // 4. bind
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(listenfd);
        return -5;
    }

    // 5. listen
    if (listen(listenfd, 511))
    {
        perror("listen");
        close(listenfd);
        return -6;
    }

    if ((epollfd = epoll_create(1)) == -1)
    {
        perror("epoll_create");
        close(listenfd);
        return -7;
    }

    event.data.fd = listenfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) == -1)
    {
        perror("epoll_ctl");
        close(listenfd);
        return -8;
    }

    events = calloc(MAXEVENT, sizeof(events));

    for (;;)
    {
        int result = epoll_wait(epollfd, events, MAXEVENT, -1);
        for (i = 0; i < result; i++)
        {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
            {
                fprintf(stderr, "socket[%d] failed", events[i].data.fd);
                close(events[i].data.fd);
                continue;
            }
            else if (listenfd == events[i].data.fd)
            {
                // 6. accept
                client_addr_size = sizeof(client_addr);
                memset(&client_addr, 0, sizeof(client_addr));
                if ((acceptfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_size)) == -1)
                {
                    perror("accept");
                    return -9;
                }

                int flags;
                if ((flags = fcntl(acceptfd, F_GETFL)) == -1)
                {
                    perror("fcntl|F_GETFL");
                    close(acceptfd);
                    continue;
                }
                if (fcntl(acceptfd, F_SETFL, flags | O_NONBLOCK) == -1)
                {
                    perror("fcntl|F_SETFL");
                    close(acceptfd);
                    continue;
                }

                event.data.fd = acceptfd;
                event.events = EPOLLIN | EPOLLET;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, acceptfd, &event) == -1)
                {
                    perror("epoll_ctl");
                    close(acceptfd);
                    continue;
                }

                fprintf(stdout, "connection from: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                continue;
            }
            else
            {
                int fd = events[i].data.fd;
                fprintf(stdout, "deal with socket[%d]\n", fd);

                for (;;)
                {
                    // 7. recv
                    memset(buf, 0, sizeof(buf));
                    num = recv(fd, buf, sizeof(buf), 0);
                    if (num == -1)
                    {
                        if (errno != EAGAIN)
                        {
                            perror("recv");
                            close(fd);
                        }
                        break;
                    }
                    else if (num == 0)
                    {
                        perror("recv");
                        close(fd);
                        break;
                    }

                    fprintf(stdout, "(socket: %d)recv[%d]: %s", fd, num, buf);

                    // 8. send
                    if ((num = send(fd, buf, strlen(buf), 0)) == -1)
                    {
                        perror("send");
                        close(fd);
                        break;
                    }

                    fprintf(stdout, "(socket: %d)send[%d]: %s", acceptfd, num, buf);
                }
            }
        }
    }

    free(events);
    close(listenfd);

    return 0;
}