#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

    fd_set readmask;
    fd_set allreads;
    FD_ZERO(&allreads);
    FD_SET(STDIN_FILENO, &allreads);

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

    FD_SET(sockfd, &allreads);

    for (;;)
    {
        readmask = allreads;

        // 4. select
        int result = select(sockfd + 1, &readmask, NULL, NULL, NULL);
        if (result == -1)
        {
            perror("select");
            close(sockfd);
            return -5;
        }
        else if (result == 0)
        {
            fprintf(stderr, "select timeouts");
            continue;
        }

        fprintf(stdout, "%d fd ready\n", result);

        // 5. recv
        if (FD_ISSET(sockfd, &readmask))
        {
            memset(buf, 0, sizeof(buf));
            num = recv(sockfd, buf, sizeof(buf), 0);
            if (num == -1)
            {
                perror("recv fail");
                close(sockfd);
                return -6;
            }
            else if (num == 0)
            {
                perror("recv zero");
                close(sockfd);
                return -6;
            }
            fprintf(stdout, "(socket: %d)recv[%d]: %s", sockfd, num, buf);
        }

        // 6. send
        if (FD_ISSET(STDIN_FILENO, &readmask))
        {
            memset(buf, 0, sizeof(buf));
            if (fgets(buf, sizeof(buf), stdin) != NULL)
            {
                if ((num = send(sockfd, buf, strlen(buf), 0)) == -1)
                {
                    perror("send");
                    close(sockfd);
                    return -7;
                }
                fprintf(stdout, "(socket: %d)send[%d]: %s", sockfd, num, buf);
            }
        }
    }

    close(sockfd);

    return 0;
}