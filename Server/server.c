#include "server.h"
#include "../Protocols/HTTP.h"
#include "../utils.h"
#include "../Shared_Mem/utils.h"
#include "../History/utils.h"
#include <pthread.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define PIPE_REQUEST "/tmp/proxy_request"
#define PIPE_RESPONSE "/tmp/proxy_response"

#define GUI_PORT 9999

// bool intercept_state = true;  // Interceptarea este activa implicit

void set_NonBlock_flag(int sockfd)
{
  // F_GETFL get value of file status flags
  int current_flags = fcntl(sockfd, F_GETFL, 0);
  // set the non-blocking property within the fd flags
  fcntl(sockfd, F_SETFL, current_flags | O_NONBLOCK);
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

struct sockaddr_in populate_socket_ForServers(int standard, const char *ip_port)
{
  struct sockaddr_in server_sock;
  memset(&server_sock, 0, sizeof(server_sock));

  // convert host to IP
  char host[256];
  int port;
  char *colon_pos = strchr(ip_port, ':');

  DIE(colon_pos == NULL, "Invalid ip_port format");

  // Extract the host (domain name) before the colon
  size_t host_len = colon_pos - ip_port;
  strncpy(host, ip_port, host_len);
  host[host_len] = '\0';

  // Extract the port number (after the colon)
  port = atoi(colon_pos + 1);
  DIE(port == 0, "Invalid port number");

  // Use getaddrinfo to resolve the domain to an IP address
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // Use IPv4
  hints.ai_socktype = SOCK_STREAM; // Use TCP

  // Resolve the host to an IP address
  int err = getaddrinfo(host, NULL, &hints, &res);
  DIE(err != 0, "getaddrinfo failed");
  printf("PORT: %d\n", port);
  server_sock.sin_family = standard;
  server_sock.sin_port = htons(port);
  // use atons to verify the integrity of the IP address
  server_sock.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
  freeaddrinfo(res);
  printf("DEBUG: IP = %s, Port = %d\n", inet_ntoa(server_sock.sin_addr), ntohs(server_sock.sin_port));
  return server_sock;
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
  while (1)
  {
    // Wait for events on either socket
    int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (event_count == -1)
    {
      perror("Error in epoll_wait");
      break;
    }

    // Process each ready socket
    for (int i = 0; i < event_count; i++)
    {
      int ready_fd = events[i].data.fd;

      if (events[i].events & EPOLLIN)
      { // Data is ready to be read
        bytes_read = recv(ready_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0)
        {
          // Connection closed or error
          break;
        }

        // Forward data to the opposite socket
        int target_socket = (ready_fd == client_socket) ? dest_socket : client_socket;
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
  close(client_socket);
  close(dest_socket);
}

void resolve_HTTP(int client_socket, char *buffer, int bytes_read)
{
  if (client_socket < 0 || buffer == NULL || bytes_read <= 0)
  {
    printf("Invalid request detected. Closing connection.\n");
    close(client_socket);
    return;
  }

  char host[1024];
  int dest_port = HTTP_PORT; // Default HTTP port
  char *url_start, *url_end, *port_str;

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
    exit(1);
  }

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

  // Close sockets
  close(dest_socket);
  close(client_socket);
}
void resolve_HTTPS(int client_socket, char *buffer)
{
  char host[1024];
  int dest_port = HTTPS_PORT; // Default HTTPS port
  char *url_start, *url_end, *port_str;
  // Extract destination host and port from CONNECT request
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
    *port_str = '\0';               // Terminate hostname before port
    dest_port = atoi(port_str + 1); // Convert port to integer
  }

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
    return;
  }

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

  // Send 200 Connection Established response to the client
  const char *connection_established = "HTTP/1.1 200 Connection Established\r\n\r\n";
  if (send(client_socket, connection_established, strlen(connection_established), 0) < 0)
  {
    perror("Error sending connection established response to client");
    close(dest_socket);
    close(client_socket);
    return;
  }

  // Relay data between client and destination server
  handle_tunnel_with_EPOLL(client_socket, dest_socket);

  // Close sockets
  close(dest_socket);
  close(client_socket);
}

// void update_intercept_state() {
//     int fd = open("/tmp/proxy_state", O_RDONLY);  // Deschide pipe-ul de stare
//     if (fd < 0) {
//         perror("Failed to open state pipe");
//         return;
//     }

//     char buffer[16];
//     ssize_t bytes_read = read(fd, buffer, sizeof(buffer));  // Citeste starea
//     close(fd);

//     if (bytes_read < 0) {
//         perror("Error reading from state pipe");
//         return;
//     }

//     buffer[bytes_read] = '\0';

//     if (strncmp(buffer, "ON", 2) == 0) {
//         intercept_state = true;
//         printf("Intercept state set to ON\n");
//     } else if (strncmp(buffer, "OFF", 3) == 0) {
//         intercept_state = false;
//         printf("Intercept state set to OFF\n");
//     } else {
//         printf("Invalid state received from pipe: %s\n", buffer);
//     }
// }

// receiver process should redirect the message
void handle_client(int client_socket)
{

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  bytes_read = recv(client_socket, buffer, CLIENT_LEN_MESS - 1, 0);
  if (bytes_read < 0)
  {
    perror("Error reading from client");
    close(client_socket);
    return;
  }
  buffer[bytes_read] = '\0';
  printf("Received request:\n%s\n", buffer);

  char method[16], url[256];
  sscanf(buffer, "%15s %255s", method, url);

  Log_AddMessage(logger, buffer);
  // Trimite informatiile catre GUI printr-un pipe
  int pipe_request_fd = open(PIPE_REQUEST, O_WRONLY);
  if (pipe_request_fd < 0)
  {
    perror("Error opening request pipe");
    close(client_socket);
    return;
  }
  set_NonBlock_flag(pipe_request_fd);
  // char gui_message[512];
  // snprintf(gui_message, sizeof(gui_message), "METHOD: %s\nURL: %s\n", method, url);
  write(pipe_request_fd, buffer, strlen(buffer));
  close(pipe_request_fd);

  int pipe_response_fd = open(PIPE_RESPONSE, O_RDONLY);
  if (pipe_response_fd < 0)
  {
    perror("Error opening response pipe");
    close(client_socket);
    return;
  }
  set_NonBlock_flag(pipe_response_fd);

  char gui_response[32];
  read(pipe_response_fd, gui_response, sizeof(gui_response));
  close(pipe_response_fd);

  if (strncmp(gui_response, "DROP", 4) == 0 && strlen(gui_response) >= 4)
  {
    printf("Request dropped by GUI\n");
    const char *error_response = "HTTP/1.1 403 Forbidden\r\n"
                                 "Content-Length: 0\r\n"
                                 "Connection: close\r\n\r\n";
    send(client_socket, error_response, strlen(error_response), 0);

    // Flush socket buffer
    char temp[512];
    while (recv(client_socket, temp, sizeof(temp) - 1, MSG_DONTWAIT) > 0)
      ;

    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    return;
  }

  printf("Request forwarded by GUI\n");

  if (strncmp(buffer, "CONNECT ", 8) == 0)
  {
    resolve_HTTPS(client_socket, buffer);
  }
  else
  {
    resolve_HTTP(client_socket, buffer, bytes_read);
  }
}

void send_to_gui(const char *message)
{
  int gui_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (gui_socket < 0)
  {
    perror("Error creating socket for GUI connection");
    return;
  }

  struct sockaddr_in gui_addr;
  memset(&gui_addr, 0, sizeof(gui_addr));
  gui_addr.sin_family = AF_INET;
  gui_addr.sin_port = htons(GUI_PORT);
  inet_pton(AF_INET, "127.0.0.1", &gui_addr.sin_addr);

  if (connect(gui_socket, (struct sockaddr *)&gui_addr, sizeof(gui_addr)) == 0)
  {
    send(gui_socket, message, strlen(message), 0);
    printf("Sent message to GUI: %s\n", message); // Log pentru verificare
    close(gui_socket);
  }
  else
  {
    perror("Failed to connect to GUI");
  }
}

void runConnection()
{
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
  {
    perror("Error creating server socket");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("Error binding server socket");
    close(server_socket);
    exit(1);
  }

  if (listen(server_socket, 10) < 0)
  {
    perror("Error listening on server socket");
    close(server_socket);
    exit(1);
  }

  printf("Proxy server listening on port %d...\n", PORT);

  while (1)
  {
    // update_intercept_state();
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0)
    {
      perror("Error accepting client connection");
      continue;
    }

    printf("Accepted a client connection\n");

    // Creeaza un thread pentru fiecare client
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, (void *)handle_client, (void *)(intptr_t)client_socket) != 0)
    {
      perror("Error creating thread");
      close(client_socket);
    }
    else
    {
      pthread_detach(thread_id); // Auto-curatare dupa terminarea thread-ului
    }
  }

  close(server_socket);
}