#include "server.h"
#include "../Cache/utils.h"

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

void handle_client(int client_sock)
{
  char message[CLIENT_LIMIT_MESS];
  while (1)
  {
    memset(message, '\0', sizeof(message));
    int bytes_received = recv(client_sock, message, sizeof(message) - 1, 0);
    if (bytes_received <= 0)
    {
      DIE(bytes_received <= 0, "message error. Close connection!");
      break;
    }
    message[bytes_received] = '\0';         // Null-terminate the received data
    printf("HTTP Request:\n%s\n", message); // Print the HTTP request

    // Optionally, process the request here or forward it to the intended server
  }
  close(client_sock);
}
