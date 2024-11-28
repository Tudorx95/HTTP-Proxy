#ifndef SERVER_H
#define SERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "../Cache/utils.h"

#define STANDARD AF_INET
#define MAX_HOSTNAME_LEN 1024
#define PORT_BROWSER 8888
#define PORT PORT_BROWSER
#define HTTP_PORT 80
#define HTTPS_PORT 443
#define IP_SERVER_ADDRESS "127.0.0.1"
#define BACKLOG 5
#define MAX_EVENTS 50

typedef struct Ready_List
{
    int fd_value;
    char *fd_name;
} Ready_List;

extern Ready_List ready_list[MAX_EVENTS];

// Ready_FD_List functions
void init_ready_fds_list();
void add_FD_Ready(int value, int fd, const char *name);

typedef struct Connection_Data
{
    int client_sock;
    char *message;
} Connection_Data;

struct sockaddr_in populate_socket(int standard, int port, const char *ip);

// main functions
void resolve_HTTP(Connection_Data *data);
void resolve_HTTPS(Connection_Data *data);
void handle_tunnel_with_EPOLL(int client_socket, int dest_socket);
// void handle_client(int client_sock);
int determine_Protocol(const char *message);
void handle_client_disconnect(void *sockfd);
int runConnection(int listen_sock);
void set_NonBlock_flag(int sockfd);
void set_timeout_sock(int socket);
int initiate_connection();

#endif
