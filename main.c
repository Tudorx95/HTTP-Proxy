#include "./Server/server.h"
#include "./Cache/utils.h"
#include "./Signal_Handlers/utils.h"
#include "utils.h"
#include <pthread.h>

void *handle_client_thread(void *client_sock_ptr)
{
    int client_sock = *(int *)client_sock_ptr;
    free(client_sock_ptr);
    handle_client(client_sock);
    close(client_sock);
    return NULL;
}

void runConnection()
{
    // initiate components
    int sockfd, client_sock, client_size;
    struct sockaddr_in server_addr, client_addr;
    char message[CLIENT_LEN_MESS];
    memset(message, '\0', sizeof(message));

    cache = create_cache();

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
    while (1)
    {
        client_size = sizeof(client_addr);
        client_sock = accept(sockfd, (struct sockaddr *)&server_addr, (socklen_t *)&client_size);
        DIE(client_sock < 0, "accept socket error");
        // method I - separate threads
        /*
                int *client_socket = malloc(sizeof(int));
                *client_socket = client_sock;
                pthread_t thread_id;
                pthread_create(&thread_id, NULL, handle_client_thread, client_socket);
                // handle_client(client_sock);
                pthread_detach(thread_id);
                // close(client_sock);
        */

        // method II - processes
        int pid = fork();
        if (pid == 0)
        {
            // handle the message
            handle_client(client_sock);
            // terminate the child process
            exit(0);
        }
    }

    close(sockfd);
    free_cache(cache);
}

int main()
{
    // signal(SIGINT, signal_caught);
    runConnection();
    return 0;
}
