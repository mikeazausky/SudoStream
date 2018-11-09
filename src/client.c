#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define BUFFER_SIZE 512

struct Node
{
    int sockfd;       // file descriptor for socket for accepting connections
    int sockfdaccept; // file descriptor for socket connected to client
};

void sm_client_terminate(int sockfd)
{
    close(sockfd);
    exit(0);
}

int sm_client_socket_create()
{
    int res = -1; // results helper
    int sockfd = 0;
    unsigned short port = 33016;

    struct sockaddr_in socket_address = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket() error");
        sm_client_terminate(sockfd);
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    socket_address.sin_port = htons(port);

    while ((res = connect(sockfd, (struct sockaddr *)&socket_address, sizeof(socket_address)))) 
    {
        perror("bind() error");
        if (res == -1)
        {
            printf("Retrying...\n");
            sleep(1);
        }
        if (res < -1)
        {
            sm_client_terminate(sockfd);
        }
    }

    return sockfd;
}

void sm_client_handshake(int sockfd)
{
    char buf[BUFFER_SIZE];

    ssize_t recvbytes = recv(sockfd, buf, sizeof(buf), 0);
    if (recvbytes < 0)
    {
        perror("recv() error");
        sm_client_terminate(sockfd);
    }
    if (recvbytes < (ssize_t)sizeof(buf))
    {
        printf("recv() error: recv() couldn't receive all bytes\n");
        sm_client_terminate(sockfd);
    }
    if (strcmp(buf, "handshake from server") != 0)
    {
        printf("error: handshake failed\n");
        sm_client_terminate(sockfd);
    }
    printf("Message received from server: %s\n", buf);

    sprintf(buf, "handshake from client");
    ssize_t sentbytes = send(sockfd, buf, sizeof(buf), 0);
    if (sentbytes < 0)
    {
        perror("send() error");
        sm_client_terminate(sockfd);
    }
    if (sentbytes < (ssize_t)sizeof(buf))
    {
        printf("send() error: send() couldn't send all bytes\n");
        sm_client_terminate(sockfd);
    }
}

int main()
{
    struct Node server = {0};
    struct Node client = {0};

    client.sockfd = sm_client_socket_create();

    sm_client_handshake(client.sockfd);

    sm_client_terminate(server.sockfd);

    return 0;
}
