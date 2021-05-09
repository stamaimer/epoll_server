#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <poll.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define INIT_SIZE 2048

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: tcp_server <ip> <port>\n");
        return -1;
    }

    fprintf(stdout, "server on %s:%d\n", argv[1], atoi(argv[2]));

    int listenfd, acceptfd, on = 1, i;
    socklen_t client_addr_size;
    struct sockaddr_in server_addr, client_addr;
    char buf[1024];
    ssize_t num;

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
    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
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

    struct pollfd event_set[INIT_SIZE];
    event_set[0].fd = listenfd;
    event_set[0].events = POLLRDNORM;

    for (i = 1; i < INIT_SIZE; i++)
    {
        event_set[i].fd = -1;
    }

    for (;;)
    {
        // 6. poll
        int result = poll(event_set, INIT_SIZE, -1);
        if (result == -1)
        {
            perror("poll");
            close(listenfd);
            return -7;
        }
        else if (result == 0)
        {
            fprintf(stderr, "poll timeouts");
            continue;
        }

        if (event_set[0].revents & POLLRDNORM)
        {
            // 7. accept
            client_addr_size = sizeof(client_addr);
            memset(&client_addr, 0, sizeof(client_addr));
            if ((acceptfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_size)) == -1)
            {
                perror("accept");
                return -8;
            }

            fprintf(stdout, "connection from: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // 8. poll acceptfd
            for (i = 1; i < INIT_SIZE; i++)
            {
                if (event_set[i].fd < 0)
                {
                    event_set[i].fd = acceptfd;
                    event_set[i].events = POLLRDNORM;
                    break;
                }
            }

            if (i == INIT_SIZE)
            {
                fprintf(stderr, "event set out of range");
                return -9;
            }

            if (--result <= 0)
            {
                continue;
            }
        }

        for (i = 1; i < INIT_SIZE; i++)
        {
            int fd;
            if ((fd = event_set[i].fd) < 0)
            {
                continue;
            }

            if (event_set[i].revents & (POLLRDNORM | POLLERR))
            {
                // 9. recv
                memset(buf, 0, sizeof(buf));
                num = recv(fd, buf, sizeof(buf), 0);
                if (num == -1)
                {
                    perror("recv");
                    close(fd);
                    return -10;
                }
                else if (num == 0 || errno == ECONNRESET)
                {
                    perror("recv");
                    close(fd);
                    event_set[i].fd = -1;
                }
                else
                {
                    fprintf(stdout, "(socket: %d)recv[%d]: %s", fd, num, buf);

                    // 10. send
                    if ((num = send(fd, buf, strlen(buf), 0)) == -1)
                    {
                        perror("send");
                        close(fd);
                        return -9;
                    }

                    fprintf(stdout, "(socket: %d)send[%d]: %s", fd, num, buf);
                }
            }
        }
    }

    close(listenfd);

    return 0;
}