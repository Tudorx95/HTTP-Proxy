#ifndef SERVER_H
#define SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "../Cache/utils.h"

#define STANDARD AF_INET
#define PORT_BROWSER 8888
#define PORT PORT_BROWSER
#define HTTP_PORT 80
#define HTTPS_PORT 443
#define IP_SERVER_ADDRESS "127.0.0.1"
#define BACKLOG 5
#define MAX_EVENTS 2

struct sockaddr_in populate_socket(int standard, int port, const char *ip);
struct sockaddr_in populate_socket_ForServers(int standard, const char *ip_port);

// receiver process
void resolve_HTTP(int client_socket, char *buffer, int bytes_read);
void resolve_HTTPS(int client_socket, char *buffer);
void handle_tunnel_with_EPOLL(int client_socket, int dest_socket);
void handle_client(int client_sock);
void runConnection();

void set_NonBlock_flag(int sockfd);

#endif
