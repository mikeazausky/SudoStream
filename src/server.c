/**
 *    SudoStream Server
 *     Music Streaming
 * ------------------------
 * 
 * Server source code for simple music streaming service
 * 
 * Author:  Thomas Brunner
 * Date:    04.12.2018
 * 
 **/

/***  INPUTS  ***/
#define SERVER_PORT 33016

/***  INCLUDES  ***/
#include "common.h"

/***  GLOBALS  ***/
struct Library library = {0};
struct State stream = {0};
struct timespec mytime = {0};
int terminatefd[100] = {0};

/***  FUNCTIONS  ***/
void ss_terminate_server(int signal)
{
    printf("--Terminating--\n");

    for (unsigned int i = 0; i < sizeof(terminatefd) / sizeof(terminatefd[0]); i++)
    {
        close(terminatefd[i]);
    }

    exit(0);
}

int ss_terminate_addfd(int fd)
{
    for (unsigned int i = 0; i < sizeof(terminatefd) / sizeof(terminatefd[0]); i++)
    {
        if (terminatefd[i] == 0)
        {
            terminatefd[i] = fd;

            return 0;
        }
    }

    return 0;
}

int ss_server_socket_create()
{
    printf("--Initialising Socket--\n");

    int sockfd = 0;
    unsigned short port = SERVER_PORT;

    struct sockaddr_in socket_address = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket() error");
        ss_terminate(0);
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0)
    {
        perror("bind() error");
        ss_terminate(0);
    }

    return sockfd;
}

int ss_server_socket_connection(int sockfd)
{
    printf("--Initialising Connection--\n");

    struct sockaddr_in sockfdclient = {0};
    socklen_t sockfdclientlen = sizeof(sockfdclient);
    int sockfdaccept = 0;

    if (listen(sockfd, 0) < 0)
    {
        perror("listen() error:");
        ss_terminate(0);
    }
    sockfdaccept = accept(sockfd, (struct sockaddr *)&sockfdclient, &sockfdclientlen);
    if (sockfdaccept < 0)
    {
        perror("accept() error");
        ss_terminate(0);
    }

    return sockfdaccept;
}

void ss_server_handshake(int sockfd)
{
    printf("--Initialising Communications--\n");

    char buf[BUFFER_SIZE] = "handshake from server";
    ssize_t sentbytes = send(sockfd, buf, sizeof(buf), 0);
    if (sentbytes < 0)
    {
        perror("send() error");
        ss_terminate(0);
    }
    if (sentbytes < (ssize_t)sizeof(buf))
    {
        printf("send() error: send() couldn't send all bytes\n");
        ss_terminate(0);
    }

    memset(buf, 0, sizeof(buf));
    ssize_t recvbytes = recv(sockfd, buf, sizeof(buf), 0);
    if (recvbytes < 0)
    {
        perror("recv() error");
        ss_terminate(0);
    }
    if (recvbytes == 0)
    {
        printf("recv() error: connection shutdown\n");
        ss_terminate(0);
    }
    if (recvbytes < (ssize_t)sizeof(buf))
    {
        printf("recv() error: recv() couldn't receive all bytes\n");
        ss_terminate(0);
    }
    if (strcmp(buf, "handshake from client") != 0)
    {
        printf("error: handshake failed\n");
        ss_terminate(0);
    }

    // Send library
    sentbytes = send(sockfd, library.text, sizeof(library.text), 0);
    if (sentbytes < 0)
    {
        perror("send() error");
        ss_terminate(0);
    }
    if (sentbytes < (ssize_t)sizeof(library.text))
    {
        printf("send() error: send() couldn't send all bytes\n");
        ss_terminate(0);
    }
}

void ss_server_stream(int sockfd)
{
    if (stream.state == 1)
    {
        char buf[BUFFER_SIZE] = "";

        ssize_t readbytes = read(stream.fd, buf, sizeof(buf));
        if (readbytes < 0)
        {
            perror("read() error");
            ss_terminate(0);
        }
        else if (readbytes == 0)
        {
            printf("--End of file--\n");
            stream.state = 0;
        }
        else
        {
            ssize_t sentbytes = send(sockfd, buf, readbytes, 0);
            if (sentbytes < 0)
            {
                perror("send() error");
                ss_terminate(0);
            }
        }
    }
}

void ss_server_stream_service(int sockfd)
{
    printf("--Starting Stream Service--\n");

    while (1)
    {
        char buf[BUFFER_SIZE] = "";

        // Check for new commands
        ssize_t recvbytes = recv(sockfd, buf, sizeof(buf), MSG_DONTWAIT);
        if (recvbytes < 0)
        {
            if (errno != EAGAIN)
            {
                perror("recv() error");
                ss_terminate(0);
            }
        }
        else if (recvbytes == 0)
        {
            printf("recv() error: connection shutdown\n");
            ss_terminate(0);
        }
        else if (strncmp(buf, "-rqst", 5) == 0)
        {
            printf("Received command %s\n", buf);

            int filenum = 0;
            if (sscanf(buf, "%*s %d", &filenum) == 1)
            {
                printf("--Streaming file %d--\n", filenum);
                // Change streaming file
                stream.state = 1;
                stream.file = filenum;

                // close last file
                if (stream.fd != 0)
                {
                    if (close(stream.fd) < 0)
                    {
                        perror("close() error");
                        ss_terminate(0);
                    }
                }

                // open new file
                if ((stream.fd = open(library.path[stream.file], O_RDONLY)) < 0)
                {
                    printf("%s", library.path[stream.file]);
                    perror("open() error");
                    ss_terminate(0);
                }
                ss_terminate_addfd(stream.fd);
            }
        }
        else if (strncmp(buf, "-stop", 5) == 0)
        {
            printf("Received command %s\n", buf);

            // Stop streaming
            stream.state = 0;
        }

        ss_server_stream(sockfd);

        // Reduce CPU usage
        mytime.tv_sec = 0;
        mytime.tv_nsec = 1000;

        nanosleep(&mytime, NULL);
    }
}

void ss_server_library()
{
    printf("--Building Music Library--\n");

    DIR *dir;
    struct dirent *ent;
    int helper = 0;
    char audio_dir[] = "./audio/";

    if ((dir = opendir(audio_dir)) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
            {
                strcpy(library.path[helper], audio_dir);
                strcat(library.path[helper], ent->d_name);

                strcpy(library.name[helper], ent->d_name);

                helper++;
            }
        }
        closedir(dir);
    }
    else
    {
        perror("opendir error");
        exit(0);
    }

    strcpy(library.text, "Music library:\n");
    for (unsigned int i = 0; i < LIBRARY_SIZE; i++)
    {
        if (strcmp(library.name[i], "") != 0)
        {
            char temps[SONG_NAME_SIZE] = "";
            sprintf(temps, " (%d) %s\n", i, library.name[i]);
            strcat(library.text, temps);
        }
    }
    printf("%s", library.text);
}

int main(int argc, char *argv[])
{
    struct Node server = {0};

    signal(SIGINT, ss_terminate);

    // Building library
    ss_server_library();

    // Create socket for incoming connections
    server.sockfd_accept = ss_server_socket_create();
    ss_terminate_addfd(server.sockfd_accept);

    while (1)
    {
        // Checking for and killing zombie child processes
        while (1)
        {
            int res = waitpid(-1, NULL, WNOHANG);
            if (res < 0)
            {
                if (errno != ECHILD)
                {
                    perror("fork() error");
                    ss_terminate(0);
                }
                break;
            }
            if (res == 0)
            {
                break;
            }
        }

        // Accepting connection
        server.sockfd_stream = ss_server_socket_connection(server.sockfd_accept);
        ss_terminate_addfd(server.sockfd_stream);

        // Creating new process for each client
        pid_t cpid = fork();
        if (cpid < 0)
        {
            perror("fork() error");
            ss_terminate(0);
        }
        if (cpid == 0)
        {
            printf("--Starting Server (socket %d)--\n", server.sockfd_stream);
            break;
        }
    }

    // Handshake
    ss_server_handshake(server.sockfd_stream);

    // Starting stream service
    ss_server_stream_service(server.sockfd_stream);

    ss_terminate(0);

    return 0;
}
