#include "./Server/server.h"
#include "./Cache/utils.h"
#include "./Signal_Handlers/utils.h"
#include "utils.h"
#include "./Shared_Mem/utils.h"
#include "./Thread/utils.h"
#include <pthread.h>

// divide processes
void delegateTasks()
{
    // set Cache as shared memory object
    create_SHM();
    init_ready_fds_list();
    // initiate the threadPool
    ThreadPool *pool;
    DIE((pool = thread_pool_init(MAX_THREADS)) == NULL, "Error creating the pool");
    if (!pool)
        return;

    // there are 3 possible events that need to be monitored
    // (listening -> accepting the connection, signal Handler, and client requests)
    struct epoll_event ev, events[MAX_EVENTS];
    int epoll_ctl_res;
    int epfd = epoll_create(MAX_EVENTS);
    if (epfd == -1)
    {
        perror("Error creating the epoll");
        return;
    }

    add_FD_Ready(0, epfd, "Epoll FD");

    int listening_sock = initiate_connection();
    if (listening_sock == -1)
        exit(EXIT_FAILURE);

    // Add listen_sock to epoll
    ev.data.fd = listening_sock;
    ev.events = EPOLLIN; // read from fd
    DIE((epoll_ctl_res = epoll_ctl(epfd, EPOLL_CTL_ADD, listening_sock, &ev)) == -1, "Error adding listen_sock to epoll");
    if (epoll_ctl_res == -1)
        exit(EXIT_FAILURE);
    // Add signal Fd to epoll
    int sigfd = initiate_SigHandler();
    if (sigfd == -1)
        exit(EXIT_FAILURE);

    ev.data.fd = sigfd;
    ev.events = EPOLLIN;
    DIE((epoll_ctl_res = epoll_ctl(epfd, EPOLL_CTL_ADD, sigfd, &ev)) == -1, "Error adding sig FD to epoll");
    if (epoll_ctl_res == -1)
        exit(EXIT_FAILURE);

    int fds, client_sock, readBytes;
    char buff[BUFFER_SIZE];
    while (1)
    {
        DIE((fds = epoll_wait(epfd, events, MAX_EVENTS, -1)) == -1, "Error waiting in epoll");
        if (fds == -1)
            exit(EXIT_FAILURE);

        for (int i = 0; i < fds; i++)
        {
            if (events[i].data.fd == listening_sock)
            {
                // accept a connection
                client_sock = runConnection(listening_sock);
                if (client_sock < 0)
                {
                    perror("Error accepting connection");
                    continue;
                }
                set_NonBlock_flag(client_sock);
                // Set socket to non-blocking mode
                // read data here and add Task to the threadPool    !!!!
                add_FD_Ready(1, client_sock, "Client_Sock");
                printf("Add client %d\n", client_sock);
                memset(buff, '\0', BUFFER_SIZE);
                readBytes = recv(client_sock, buff, BUFFER_SIZE - 1, 0);
                if (readBytes <= 0)
                {
                    if (readBytes == 0)
                        printf("Client %d closed connection\n", client_sock);
                    else
                        perror("Error receiving client message");

                    // add_FD_Ready(1, client_sock, "Reading OP");
                    //  int *arg = malloc(sizeof(int));
                    //  if (!arg)
                    //  {
                    //      perror("Failed to allocate memory");
                    //      continue;
                    //  }
                    //  *arg = events[i].data.fd;
                    //  close connection here    !!!!!
                    //  thread_pool_add_Task(pool, handle_client_disconnect, arg);
                    close(client_sock);
                    // Remove from epoll
                    // epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);

                    if (readBytes == 0)
                        fflush(stdout);
                }
                else
                {
                    buff[readBytes] = '\0';
                    // Create connection data structure
                    Connection_Data *newConn = (Connection_Data *)malloc(sizeof(Connection_Data));
                    if (!newConn)
                    {
                        perror("Failed to allocate connection data");
                        continue;
                    }
                    // add message in cache !
                    // cacheConstructor(buff);

                    newConn->client_sock = client_sock; // Use actual client socket
                    newConn->message = strdup(buff);    // Use strdup for proper memory allocation
                    if (!newConn->message)
                    {
                        free(newConn);
                        perror("Failed to allocate message buffer");
                        continue;
                    }

                    // Determine protocol and handle request
                    void *handle_client = determine_Protocol(buff) ? &resolve_HTTP : &resolve_HTTPS;
                    thread_pool_add_Task(pool, handle_client, newConn);
                }
                // char event_info[64];
                // snprintf(event_info, sizeof(event_info), "Event fd: %d", events[i].data.fd);
                // add_FD_Ready(0, events[i].data.fd, event_info);
            }
            else if (events[i].data.fd == sigfd)
            {
                // handle the signal
                struct signalfd_siginfo fdsi;

                // Check for signals using signalfd
                read(sigfd, &fdsi, sizeof(fdsi));
                signal_caught(fdsi.ssi_signo);
            }
            // else
            // {
            //     // handle the client message
            //     memset(buff, '\0', BUFFER_SIZE);
            //     readBytes = recv(events[i].data.fd, buff, BUFFER_SIZE - 1, 0);

            //     if (readBytes <= 0)
            //     {
            //         if (readBytes == 0)
            //             printf("Client %d closed connection\n", events[i].data.fd);
            //         else
            //             perror("Error receiving client message");

            //         add_FD_Ready(1, events[i].data.fd, "Reading OP");
            //         int *arg = malloc(sizeof(int));
            //         if (!arg)
            //         {
            //             perror("Failed to allocate memory");
            //             continue;
            //         }
            //         *arg = events[i].data.fd;
            //         // close connection here    !!!!!
            //         thread_pool_add_Task(pool, handle_client_disconnect, arg);

            //         // Remove from epoll
            //         epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);

            //         if (readBytes == 0)
            //             fflush(stdout);
            //     }
            //     else
            //     {
            //         buff[readBytes] = '\0';
            //         // Create connection data structure
            //         Connection_Data *newConn = (Connection_Data *)malloc(sizeof(Connection_Data));
            //         if (!newConn)
            //         {
            //             perror("Failed to allocate connection data");
            //             continue;
            //         }
            //         // add message in cache !
            //         // cacheConstructor(buff);

            //         newConn->client_sock = events[i].data.fd; // Use actual client socket
            //         newConn->message = strdup(buff);          // Use strdup for proper memory allocation
            //         if (!newConn->message)
            //         {
            //             free(newConn);
            //             perror("Failed to allocate message buffer");
            //             continue;
            //         }

            //         // Determine protocol and handle request
            //         void *handle_client = determine_Protocol(buff) ? &resolve_HTTP : &resolve_HTTPS;
            //         thread_pool_add_Task(pool, handle_client, newConn);
            //     }

            //     char event_info[64];
            //     snprintf(event_info, sizeof(event_info), "Event fd: %d", events[i].data.fd);
            //     add_FD_Ready(0, events[i].data.fd, event_info);
            // }
        }
    }

    // Cleanup
    for (int i = 0; i < MAX_EVENTS; i++)
    {
        if (ready_list[i].fd_value == 1)
        {
            int *newfd = malloc(sizeof(int));
            if (newfd)
            {
                *newfd = ready_list[i].fd_value;
                thread_pool_add_Task(pool, handle_client_disconnect, newfd);
                ready_list[i].fd_value = 0;
            }
        }
    }

    close(listening_sock);
    close(sigfd);
    close(epfd);
}

int main()
{
    delegateTasks();
    return 0;
}
