#include "server.h"
#include "../Cache/utils.h"
#include "../Protocols/HTTP.h"
#include "../utils.h"

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
  char message[CLIENT_LEN_MESS];
  while (1)
  {
    memset(message, '\0', sizeof(message));
    int bytes_received = recv(client_sock, message, sizeof(message) - 1, 0);

    if (bytes_received <= 0)
    {
      // printf("%d\n", bytes_received);
      DIE(bytes_received < 0, "message error. Close connection!");
      // else the client closed the connection
      break;
    }
    message[bytes_received] = '\0'; // Null-terminate the received data
    printf("%s\n", message);        // Print the HTTP request
    //void *http_message = HTTP_constructor(message);

    char *cached_response = retrieve_message(cache, message);
      if (cached_response) {
            printf("Cache HIT: Found message in cache for request: %s\n", message);
            printf("Cached response: %s\n", cached_response);
            free(cached_response);
        } else {
            printf("Cache MISS: Message not found in cache for request: %s\n", message);
            store_message(cache, message, "Simulated response: This is the cached content for GET / HTTP/1.1");
            printf("Stored new message in cache: %s\n", message);
        }
    // Optionally, process the request here or forward it to the intended server
  }
  close(client_sock);
}
