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
        fprintf(stderr, "usage: tcp_client <ip> <port>\n");
        return -1;
    }

    fprintf(stdout, "connection to %s:%d\n", argv[1], atoi(argv[2]));

    int sockfd, i;
    struct sockaddr_in addr;
    char buf[1024];
    ssize_t num;

    // 1. init server_addr
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], &addr.sin_addr) == 0)
    {
        perror("inet_aton");
        return -2;
    }

    // 2. create socket
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -3;
    }

    // 3. connection
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        return -4;
    }

    for (;;)
    {
        // 4. send
        memset(buf, 0, sizeof(buf));
        for (i = 0; (buf[i] = getchar()) != '\n'; i++)
        {
            ;
        }
        if ((num = send(sockfd, buf, strlen(buf), 0)) == -1)
        {
            perror("send");
            close(sockfd);
            return -5;
        }

        fprintf(stdout, "(socket: %d)send[%d]: %s", sockfd, num, buf);

        // 5. recv
        memset(buf, 0, sizeof(buf));
        if ((num = recv(sockfd, buf, sizeof(buf), 0)) == -1)
        {
            perror("recv");
            close(sockfd);
            return -6;
        }

        fprintf(stdout, "(socket: %d)recv[%d]: %s", sockfd, num, buf);

        if (strncmp(buf, "quit", strlen("quit")) == 0)
        {
            fprintf(stdout, "client quit\n");
            break;
        }
    }

    close(sockfd);

    return 0;
}