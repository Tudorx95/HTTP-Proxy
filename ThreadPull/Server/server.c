#include "server.h"
#include "../Protocols/HTTP.h"
#include "../utils.h"
#include "../Shared_Mem/utils.h"
#include <errno.h>

Ready_List ready_list[MAX_EVENTS];

int wait_for_connection(int sockfd, int timeout_sec)
{
  fd_set write_fds;
  struct timeval timeout;

  FD_ZERO(&write_fds);
  FD_SET(sockfd, &write_fds);

  timeout.tv_sec = timeout_sec;
  timeout.tv_usec = 0;

  // Wait for the connection to complete
  int result = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
  if (result > 0)
  {
    // Check if connection was successful
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0)
    {
      return -1;
    }
    return 0;
  }
  return -1;
}

struct sockaddr_in populate_socket(int standard, int port, const char *ip)
{
  struct sockaddr_in server_sock;
  memset(&server_sock, 0, sizeof(server_sock));
  server_sock.sin_family = standard;
  server_sock.sin_port = htons(port);
  // use atons to verify the integrity of the IP address
  DIE(inet_aton(ip, &server_sock.sin_addr) == 0, "IP conversion error");

  return server_sock;
}

void init_ready_fds_list()
{
  for (int i = 0; i < (int)(sizeof(ready_list) / sizeof(ready_list[0])); i++)
  {
    ready_list[i].fd_value = 0;
    ready_list[i].fd_name = NULL;
  }
}

void add_FD_Ready(int value, int fd, const char *name)
{
  ready_list[fd].fd_value = value;
  ready_list[fd].fd_name = malloc((strlen(name) + 1) * sizeof(char));
  strcpy(ready_list[fd].fd_name, name);
}

void handle_tunnel_with_EPOLL(int client_socket, int dest_socket)
{
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  // Initialize epoll
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1)
  {
    perror("Error creating epoll instance");
    close(dest_socket);
    close(client_socket);
    return;
  }

  // Add client_socket to epoll
  struct epoll_event event;
  event.events = EPOLLIN; // Monitor for input readiness
  event.data.fd = client_socket;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
  {
    perror("Error adding client socket to epoll");
    close(epoll_fd);
    return;
  }

  // Add dest_socket to epoll
  event.data.fd = dest_socket;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dest_socket, &event) == -1)
  {
    perror("Error adding destination socket to epoll");
    close(epoll_fd);
    return;
  }

  struct epoll_event events[MAX_EVENTS];
  int event_count, ready_fd, target_socket, i;
  while (1)
  {
    event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (event_count == -1)
    {
      perror("Error in epoll_wait");
      break;
    }

    // Process each ready socket
    for (i = 0; i < event_count; i++)
    {
      ready_fd = events[i].data.fd;

      if (events[i].events & EPOLLIN)
      { // Data is ready to be read
        bytes_read = recv(ready_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0)
        {
          // Connection closed or error
          break;
        }

        // Forward data to the opposite socket
        target_socket = (ready_fd == client_socket) ? dest_socket : client_socket;
        if (send(target_socket, buffer, bytes_read, 0) < 0)
        {
          perror("Error forwarding data");
          break;
        }
      }
    }
  }

  // Clean up
  close(epoll_fd);
  close(dest_socket);
  close(client_socket);
}

void resolve_HTTP(Connection_Data *data)
{
  printf("Resolve HTTP: %ld\n", pthread_self());
  char host[MAX_HOSTNAME_LEN];
  int dest_port = HTTP_PORT; // Default HTTP port
  char *url_start, *url_end, *port_str;
  char *buffer = data->message;
  int client_socket = data->client_sock;

  // Extract destination host and path from the request
  url_start = strstr(buffer, "Host: ");
  if (!url_start)
  {
    fprintf(stderr, "No Host header found\n");
    close(client_socket);
    return;
  }
  url_start += 6; // Move past "Host: "
  url_end = strstr(url_start, "\r\n");
  if (!url_end)
  {
    fprintf(stderr, "Invalid Host header\n");
    close(client_socket);
    return;
  }

  // Copy the host portion, separating out any port if it exists
  snprintf(host, sizeof(host), "%.*s", (int)(url_end - url_start), url_start);

  port_str = strchr(host, ':'); // Look for port separator
  if (port_str)
  {
    *port_str = '\0';               // Terminate hostname string before port
    dest_port = atoi(port_str + 1); // Convert port to integer if specified
  }
  printf("Host Domain Name: %s Port: %s\n", host, port_str);

  // Resolve hostname to IP address
  struct hostent *server = gethostbyname(host);
  if (!server)
  {
    fprintf(stderr, "Error: No such host %s\n", host);
    close(client_socket);
    return;
  }

  // Create socket for connecting to the destination server
  int dest_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (dest_socket < 0)
  {
    perror("Error opening destination socket");
    close(client_socket);
    exit(EXIT_FAILURE);
  }
  // set_timeout_sock(dest_socket);

  // Configure destination server address
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  memcpy(&dest_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  dest_addr.sin_port = htons(dest_port);

  // Connect to the destination server
  if (connect(dest_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
  {
    perror("Error connecting to destination server");
    close(dest_socket);
    close(client_socket);
    return;
  }
  // set_NonBlock_flag(dest_socket);

  printf("Connection to %d created!\n", dest_socket);

  // set_NonBlock_flag(dest_socket);

  int bytes_read = strlen(buffer);
  // Forward client request to destination server
  if (send(dest_socket, buffer, bytes_read, 0) < 0)
  {
    perror("Error forwarding request to destination server");
    close(dest_socket);
    close(client_socket);
    return;
  }

  // Receive response from destination server and send it back to the client
  while ((bytes_read = recv(dest_socket, buffer, BUFFER_SIZE, 0)) > 0)
  {
    if (send(client_socket, buffer, bytes_read, 0) < 0)
    {
      perror("Error sending response to client");
      break;
    }
  }
  printf("Close connections for client_sock: %d\n", client_socket);
  // Close sockets
  close(dest_socket);
  close(client_socket);
  printf("Finish HTTP: %ld\n", pthread_self());
}
void resolve_HTTPS(Connection_Data *data)
{
  printf("Resolve HTTPS: %ld\n", pthread_self());
  char host[1024];
  int dest_port = HTTPS_PORT; // Default HTTPS port
  char *url_start, *url_end, *port_str;
  char *buffer = data->message;
  int client_socket = data->client_sock;

  // Extract destination host and port
  url_start = buffer + 8;
  url_end = strchr(url_start, ' ');
  if (!url_end)
  {
    fprintf(stderr, "Invalid CONNECT request\n");
    close(client_socket);
    return;
  }
  snprintf(host, sizeof(host), "%.*s", (int)(url_end - url_start), url_start);
  port_str = strchr(host, ':');
  if (port_str)
  {
    *port_str = '\0';
    dest_port = atoi(port_str + 1);
  }

  // Resolve hostname to IP
  struct hostent *server = gethostbyname(host);
  if (!server)
  {
    fprintf(stderr, "Error: No such host %s\n", host);
    close(client_socket);
    return;
  }
  // Create socket for destination server
  int dest_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (dest_socket < 0)
  {
    perror("Error opening destination socket");
    close(client_socket);
    return;
  }
  // set_timeout_sock(dest_socket);

  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  memcpy(&dest_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  dest_addr.sin_port = htons(dest_port);

  // Connect to the destination server
  if (connect(dest_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
  {
    perror("Error connecting to destination server");
    close(dest_socket);
    close(client_socket);
    return;
  }
  // set_NonBlock_flag(dest_socket);

  //  Send 200 Connection Established response
  const char *connection_established = "HTTP/1.1 200 Connection Established\r\n\r\n";
  if (send(client_socket, connection_established, strlen(connection_established), 0) < 0)
  {
    perror("Error sending connection established response");
    close(dest_socket);
    close(client_socket);
    return;
  }

  // Tunnel data
  handle_tunnel_with_EPOLL(client_socket, dest_socket);
  printf("Finish HTTPS: %ld\n", pthread_self());
}

int determine_Protocol(const char *message)
{
  // Check if the request is an HTTPS CONNECT request
  if (strncmp(message, "CONNECT ", 8) != 0)
    // http request
    return 1;

  // https request
  return 0;
}

int initiate_connection()
{
  int server_socket;
  struct sockaddr_in server_addr;

  // Create server socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
  {
    perror("Error creating server socket");
    exit(1);
  }

  // Configure server address
  server_addr = populate_socket(AF_INET, PORT, IP_SERVER_ADDRESS);

  int opt = 1;
  // if another instance is using the specified port, reuse it by setting SOL_SOCKET for working with socket API
  // and SO_REUSEADDR to reuse the port
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Bind server socket
  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("Error binding server socket");
    close(server_socket);
    exit(1);
  }

  // Listen for incoming connections
  if (listen(server_socket, 10) < 0)
  {
    perror("Error listening on server socket");
    close(server_socket);
    exit(1);
  }
  printf("Proxy server listening on port %d...\n", PORT);
  return server_socket;
}

void set_NonBlock_flag(int sockfd)
{
  // F_GETFL get value of file status flags
  int current_flags = fcntl(sockfd, F_GETFL, 0);
  // set the non-blocking property within the fd flags
  fcntl(sockfd, F_SETFL, current_flags | O_NONBLOCK);
}

void set_timeout_sock(int sock)
{
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

int runConnection(int server_socket)
{
  // Accept and handle client connections
  struct sockaddr_in client_addr;
  int client_socket;
  socklen_t client_len = sizeof(client_addr);
  client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
  if (client_socket < 0)
  {
    perror("Error accepting client connection");
    return -1;
  }
  set_NonBlock_flag(client_socket);
  return client_socket;
}

void handle_client_disconnect(void *sockfd)
{
  printf("Disconnecting the thread: %ld\n", pthread_self());
  // printf("Close client socket: %d\n", *(int *)sockfd);
  //  libereate cache here

  close(*(int *)(sockfd));
  free(sockfd);
}