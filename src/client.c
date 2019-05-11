/**
 *    SudoStream Client
 *     Music Streaming
 * ------------------------
 * 
 * Client source code for simple music streaming service
 * 
 * Author:  Thomas Brunner
 * Date:    04.12.2018
 * 
 **/

/***  INPUTS  ***/
#define SERVER_ADDRESS "127.0.0.1" // don't forget quotation marks!
#define SERVER_PORT 33016

/***  INCLUDES  ***/
#define CLIENT
#include "common.h"

/***  GLOBALS  ***/
struct Cache cache = {0};
struct State stream = {0};
struct State play = {0};
struct Library library = {0};
struct timespec mytime = {0};
int terminatefd[100] = {0};
libvlc_instance_t *my_instance;
libvlc_media_t *mediafile;
libvlc_media_player_t *my_player;

/***  FUNCTIONS  ***/
void ss_terminate_client(int signal)
{
    printf("--Terminating--\n");

    if (unlink(cache.name) < 0)
    {
        perror("unlickat() error");
    }

    libvlc_media_release(mediafile);
    libvlc_media_player_release(my_player);
    libvlc_release(my_instance);

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

void ss_client_cache_new()
{
    printf("--Initialising Cache--\n");

    srand(time(NULL));
    cache.num = rand();

    sprintf(cache.name, "./cache/cache%d.mp3", cache.num);

    if ((cache.fd = open(cache.name, O_RDWR | O_APPEND | O_CREAT | O_EXCL, 0666)) < 0)
    {
        if (errno == EEXIST)
        {
            printf("Error: Client with this ID already exists.\n");
        }
        perror("open() error");
        exit(0);
    }
    ss_terminate_addfd(cache.fd);
}

void ss_client_cache_reset()
{
    if (close(cache.fd) < 0)
    {
        perror("close() error");
        ss_terminate(0);
    }

    if ((cache.fd = open(cache.name, O_RDWR | O_APPEND | O_TRUNC, 0666)) < 0)
    {
        if (errno == EEXIST)
        {
            printf("Error: Client with this ID already exists.\n");
        }
        perror("open() error");
        ss_terminate(0);
    }
}

int ss_client_socket_create()
{
    printf("--Initialising Socket--\n");
    int res = -1; // results helper
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
    socket_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    socket_address.sin_port = htons(port);

    while ((res = connect(sockfd, (struct sockaddr *)&socket_address, sizeof(socket_address))))
    {
        perror("connect() error");
        if (res == -1)
        {
            printf("Retrying...\n");
            sleep(1);
        }
        if (res < -1)
        {
            ss_terminate(0);
        }
    }

    return sockfd;
}

void ss_client_handshake(int sockfd)
{
    printf("--Initialising Communications--\n");
    char buf[BUFFER_SIZE] = {0};

    ssize_t recvbytes = recv(sockfd, buf, sizeof(buf), 0);
    if (recvbytes < 0)
    {
        perror("recv() error");
        ss_terminate(0);
    }
    if (recvbytes < (ssize_t)sizeof(buf))
    {
        printf("recv() error: recv() couldn't receive all bytes\n");
        ss_terminate(0);
    }
    if (strcmp(buf, "handshake from server") != 0)
    {
        printf("error: handshake failed\n");
        ss_terminate(0);
    }
    if (recvbytes == 0)
    {
        printf("recv() error: connection shutdown\n");
        ss_terminate(0);
    }

    sprintf(buf, "handshake from client");
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

    recvbytes = recv(sockfd, library.text, sizeof(library.text), 0);
    if (recvbytes < 0)
    {
        perror("recv() error");
        ss_terminate(0);
    }
    if (recvbytes < (ssize_t)sizeof(library.text))
    {
        printf("recv() error: recv() couldn't receive all bytes\n");
        ss_terminate(0);
    }
    if (recvbytes == 0)
    {
        printf("recv() error: connection shutdown\n");
        ss_terminate(0);
    }

    char tempsource[sizeof(library.text)];
    strcpy(tempsource, library.text);
    char *tempstring;

    tempstring = strtok(tempsource, "M\n");
    for (unsigned int i = 0; i < LIBRARY_SIZE; i++)
    {
        tempstring = strtok(NULL, "() \n");

        if (tempstring == NULL)
        {
            break;
        }

        //library.fd[i] = strtol(tempstring, NULL, 10);

        tempstring = strtok(NULL, "() \n");

        if (tempstring == NULL)
        {
            break;
        }

        strcpy(library.name[i], tempstring);
    }
}

int ss_client_stream(int sockfd)
{
    if (stream.state == 1)
    {
        char buf[BUFFER_SIZE] = "";

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
        else
        {
            // Write to cache
            ssize_t writebytes = write(cache.fd, buf, recvbytes);
            if (writebytes < 0)
            {
                perror("read() error");
                exit(0);
            }

            return writebytes;
        }
    }

    return 0;
}

void ss_client_usage_reminder(int type)
{
    if (type == 0)
    {
        printf("Unknown command. ");
    }

    printf("Available commands:\n \"-play {song number}\"\n \"-stop\"\n \"-lib\"\n");
}

void ss_client_stream_service(int sockfd)
{
    printf("--Starting Stream Service--\n");

    printf("%s", library.text);

    ss_client_usage_reminder(1);

    while (1)
    {
        char buf[BUFFER_SIZE] = "";
        struct pollfd pollhelper = {0}; // Helper for poll function
        pollhelper.fd = STDIN_FILENO;   // Where poll should check
        pollhelper.events = POLLIN;     // Events that the poll reacts to
        int res = 0;

        res = poll(&pollhelper, 1, 0); // Checking stdin for new input

        if (pollhelper.revents != 1 && pollhelper.revents != 0) // Unexpected condition (?)
        {
            printf("poll() error: check revents\n");
            ss_terminate(0);
        }
        if (res > 0) // Messages ready to be read
        {
            // Process input
            if (fgets(buf, sizeof(buf), stdin) != 0)
            {
                if (strncmp(buf, "-play", 5) == 0)
                {
                    int streamfile = 0;
                    if (sscanf(buf, "%*s %d", &streamfile) == 1)
                    {
                        int fileexists = 0;

                        for (unsigned int i = 0; i < LIBRARY_SIZE; i++)
                        {
                            if (strcmp(library.name[streamfile], "") != 0)
                            {
                                fileexists = 1;
                                break;
                            }
                        }

                        if (fileexists)
                        {
                            libvlc_media_player_release(my_player);

                            sprintf(buf, "%s", "-stop");
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

                            // clear buffer
                            while (1)
                            {
                                int recvbytes = ss_client_stream(sockfd);
                                if (recvbytes == 0)
                                {
                                    sleep(1);
                                    recvbytes = ss_client_stream(sockfd);
                                    if (recvbytes == 0)
                                    {
                                        break;
                                    }
                                }
                            }

                            // clear cache
                            ss_client_cache_reset();

                            play.file = streamfile;

                            // Sending request to server
                            sprintf(buf, "%s %d", "-rqst", play.file);

                            sentbytes = send(sockfd, buf, sizeof(buf), 0);
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

                            stream.state = 1;

                            sleep(1);
                            for (int i = 0; i < (int)(320e3f * 5.0f / (float)BUFFER_SIZE); i++)
                            {
                                ss_client_stream(sockfd);
                            }
                            sleep(1);

                            my_player = libvlc_media_player_new_from_media(mediafile);
                            libvlc_media_player_play(my_player);
                        }
                        else
                        {
                            printf("File not found.\n");
                        }
                    }
                    else
                    {
                        ss_client_usage_reminder(0);
                    }
                }
                else if (strncmp(buf, "-stop", 5) == 0)
                {
                    //printf("Pausing something\n");
                    libvlc_media_player_pause(my_player);
                }
                else if (strncmp(buf, "-lib", 4) == 0)
                {
                    printf("%s", library.text);
                }
                else
                {
                    ss_client_usage_reminder(0);
                }
            }
        }
        else if (res == 0) // No new messages
        {
            // Continue streaming
            ss_client_stream(sockfd);
        }
        else
        {
            perror("poll() error");
            ss_terminate(0);
        }

        // Reduce CPU usage
        mytime.tv_sec = 0;
        mytime.tv_nsec = 1000;

        nanosleep(&mytime, NULL);
    }
}

void ss_client_player_init()
{
    printf("--Initialising Media Player--\n");
    my_instance = libvlc_new(0, NULL);
    if (my_instance == NULL)
    {
        printf("libvlc_new() error\n");
        ss_terminate(0);
    }

    mediafile = libvlc_media_new_path(my_instance, cache.name);

    my_player = libvlc_media_player_new_from_media(mediafile);
}

int main(int argc, char *argv[])
{
    struct Node client = {0};

    signal(SIGINT, ss_terminate);

    ss_client_cache_new();

    ss_client_player_init();

    // Creating socket for stream
    client.sockfd_stream = ss_client_socket_create();
    ss_terminate_addfd(client.sockfd_stream);

    // Handshake
    ss_client_handshake(client.sockfd_stream);

    // Starting stream service
    ss_client_stream_service(client.sockfd_stream);

    ss_terminate(0);

    return 0;
}
