/**
 *    SudoStream Common
 *     Music Streaming
 * ------------------------
 * 
 * Common declarations
 * 
 * Author:  Thomas Barthel Brunner 
 * E-mail:  thomasbbrunner@gmail.com
 * Date:    04.12.2018
 *    
 **/

#ifndef COMMON_H_
#define COMMON_H_

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <sys/wait.h>

#ifdef CLIENT
#define ss_terminate ss_terminate_client
#include <vlc/vlc.h>
#else
#define ss_terminate ss_terminate_server
#endif

#define BUFFER_SIZE 512    // size of stream buffer
#define LIBRARY_SIZE 100   // max number of songs inside the library
#define SONG_NAME_SIZE 100 // max size of song names

struct Node
{
    int sockfd_stream; // file descriptor for socket for streaming
    int sockfd_accept; // file descriptor for socket for connection
};

struct Library
{
    char name[LIBRARY_SIZE][SONG_NAME_SIZE];
    char path[LIBRARY_SIZE][SONG_NAME_SIZE];
    char text[BUFFER_SIZE];
};

struct Cache
{
    int num;
    int fd;
    char name[100];
};

struct State
{
    int state;
    int file;
    int fd;
};

#endif