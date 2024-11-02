#ifndef SERVER_H
#define SERVER_H
#include "../utils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define STANDARD AF_INET
#define PORT_BROWSER 8888
#define IP_SERVER_ADDRESS "127.0.0.1"
#define BACKLOG 5

struct sockaddr_in populate_socket(int standard, int port, const char *ip);
void handle_client(int client_sock);

#endif
