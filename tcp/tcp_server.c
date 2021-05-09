#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: tcp_server <ip> <port>\n");
        return -1;
    }

    fprintf(stdout, "server on %s:%d\n", argv[1], atoi(argv[2]));

    int listenfd, acceptfd, on = 1;
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

    for (;;)
    {
        fprintf(stdout, "(socket: %d)wait for new connection\n", listenfd);
        // 6. accept
        client_addr_size = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));
        if ((acceptfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_size)) == -1)
        {
            perror("accept");
            return -7;
        }

        fprintf(stdout, "connection from: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        for (;;)
        {
            // 7. recv
            memset(buf, 0, sizeof(buf));
            num = recv(acceptfd, buf, sizeof(buf), 0);
            if (num == -1)
            {
                perror("recv");
                close(acceptfd);
                return -8;
            }
            else if (num == 0)
            {
                perror("recv");
                close(acceptfd);
                return -8;
            }

            fprintf(stdout, "(socket: %d)recv[%d]: %s", acceptfd, num, buf);

            // 8. send
            if ((num = send(acceptfd, buf, strlen(buf), 0)) == -1)
            {
                perror("send");
                close(acceptfd);
                return -9;
            }

            fprintf(stdout, "(socket: %d)send[%d]: %s", acceptfd, num, buf);

            if (strncmp(buf, "quit", strlen("quit")) == 0)
            {
                fprintf(stdout, "server quit\n");
                break;
            }
        }

        close(acceptfd);
    }

    close(listenfd);

    return 0;
}