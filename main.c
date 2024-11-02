#include "./Server/server.h"
#include "./Cache/utils.h"
#include "./Signal_Handlers/utils.h"

void runConnection()
{
    // initiate components
    int sockfd, client_sock, client_size;
    struct sockaddr_in server_addr, client_addr;
    char message[CLIENT_LIMIT_MESS];
    memset(message, '\0', sizeof(message));

    // create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    DIE(sockfd < 0, "socket error");
    // Set port and IP address same as server
    server_addr = populate_socket(STANDARD, PORT_BROWSER, IP_SERVER_ADDRESS);

    // Use the setsockpt call to reuse the port in case any process is already using it
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Bind to the port and IP
    DIE(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0, "error binding to the port");
    // listen for clients
    DIE(listen(sockfd, BACKLOG) < 0, "listening error");

    // Accept incoming connection from the Web Browser

    client_size = sizeof(client_addr);
    client_sock = accept(sockfd, (struct sockaddr *)&server_addr, (socklen_t *)&client_size);
    DIE(client_sock < 0, "accept socket error");

    handle_client(client_sock);

    close(client_sock);
    close(sockfd);
}

int main()
{
    // signal(SIGINT, signal_caught);
    runConnection();
    return 0;
}
