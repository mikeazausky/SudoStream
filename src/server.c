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

void sm_server_terminate(int sockfd)
{
    close(sockfd);
    exit(0);
}

int sm_server_socket_create()
{
    int sockfd = 0;
    unsigned short port = 33016;

    struct sockaddr_in socket_address = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket() error");
        sm_server_terminate(sockfd);
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    socket_address.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0)
    {
        perror("bind() error");
        sm_server_terminate(sockfd);
    }

    return sockfd;
}

int sm_server_socket_connection(int sockfd)
{
    struct sockaddr_in sockfdclient = {0};
    socklen_t sockfdclientlen = sizeof(sockfdclient);
    int sockfdaccept = 0;

    if (listen(sockfd, 0) < 0)
    {
        perror("listen() error:");
        sm_server_terminate(sockfd);
    }
    sockfdaccept = accept(sockfd, (struct sockaddr *)&sockfdclient, &sockfdclientlen);
    if (sockfdaccept < 0)
    {
        perror("accept() error");
        sm_server_terminate(sockfd);
    }

    return sockfdaccept;
}

void sm_server_handshake(int sockfd)
{
    char buf[BUFFER_SIZE] = "handshake from server";
    ssize_t sentbytes = send(sockfd, buf, sizeof(buf), 0);
    if (sentbytes < 0)
    {
        perror("send() error");
        sm_server_terminate(sockfd);
    }
    if (sentbytes < (ssize_t)sizeof(buf))
    {
        printf("send() error: send() couldn't send all bytes\n");
        sm_server_terminate(sockfd);
    }

    sprintf(buf, "");
    ssize_t recvbytes = recv(sockfd, buf, sizeof(buf), 0);
    if (recvbytes < 0)
    {
        perror("recv() error");
        sm_server_terminate(sockfd);
    }
    if (recvbytes < (ssize_t)sizeof(buf))
    {
        printf("recv() error: recv() couldn't receive all bytes\n");
        sm_server_terminate(sockfd);
    }
    if (strcmp(buf, "handshake from client") != 0)
    {
        printf("error: handshake failed\n");
        sm_server_terminate(sockfd);
    }
    printf("Message received from client: %s\n", buf);
}

void sm_server_listen_commands(int sockfd)
{
    // Communication between the server and client is based on a protocol:
    //      * Each command is preceded by a dash "-"
    //      * Each command is composed of four letters
    //      * Available commands:
    //          -rqst (request file for streaming)
    //          -stop (stop stream)

    while (1)
    {
        char buf[BUFFER_SIZE] = "";

        ssize_t recvbytes = recv(sockfd, buf, sizeof(buf), 0);

        if (strncmp(buf, "-rqst", 5))
        {
        }
        else if (strncmp(buf, "-stop", 5))
        {
        }
        else
        {
            printf("error: unknown command\n");
            sm_server_terminate(sockfd);
        }
    }
}

int main()
{
    struct Node server = {0};
    struct Node client = {0};

    server.sockfdaccept = sm_server_socket_create();

    server.sockfd = sm_server_socket_connection(server.sockfdaccept);

    sm_server_handshake(server.sockfd);

    sm_server_listen_commands(server.sockfd);

    sm_server_terminate(server.sockfd);

    return 0;
}
