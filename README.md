# HTTP Proxy App (Proxy Eye)

Proxy Eye is a C based application designed to manage all network traffic between a web browser and destination
servers. Users can interact behind the browser resources via a desktop application, giving insights about the
specific assets required to render a website correctly.

## Table of Contents

- [HTTP Proxy App (Proxy Eye)](#http-proxy-app-proxy-eye)
  - [Table of Contents](#table-of-contents)
  - [What is an open proxy server?](#what-is-an-open-proxy-server)
  - [Project Construction Diagram](#project-construction-diagram)
  - [Proxy Server Functionalities](#proxy-server-functionalities)
    - [Internal functionalities](#internal-functionalities)
  - [GUI Functionalities](#gui-functionalities)
  - [Usage](#usage)
  - [Contributors](#contributors)


- [What is an open proxy server?](#project_overview)
- [Project Construction Diagram](#project_construction)
- [Proxy Server Functionalities](#server_app)
- [GUI Functionalities](#gui_app)
- [Usage](#run_app)
- [Contributors](#contributors)


## What is an open proxy server?

A proxy server is a specific type of server application used to forward traffic between the web browser
and the actual destination server. In an open **_HTTP_** proxy server, the resources are managed by
independent clients and it is distributed, or not, according to their actions.

![Principles of a proxy server - Wikipedia](https://upload.wikimedia.org/wikipedia/commons/b/bb/Proxy_concept_en.svg)

The Proxy Eye application is intended for educational purposes, representing the **_HTTP_** messages in
their pure form, alongside all its specifications.

## Project Construction Diagram

![Proxy Server Connections](Images/Proxy%20Diagram.jpg)

## Proxy Server Functionalities

1. TCP Socket -> Packets from web browser are captured via a TCP socket
2. Processing requests -> The messages will be stored in a special data structure
<<<<<<< HEAD
intended to mannage ***HTTP*** messages. 
3. Register messages -> using a cache list data structure.
The data structure is necessarily for managing the packet flow from the listener to
the GUI application. 
When there are enough packets send, the server will store those messages in a cache
list.

<!--### Implementation of CACHE Management Unit (CMU)
Cache history list is a list based on FIFO principle that contains every new message 
=======
   intended to mannage **_HTTP_** messages.
3. Register messages -> using a cache list data structure and a cache history list.
   The data structure is necessarily for managing the packet flow from the listener to
   the GUI application.
   When there are enough packets send, the server will store those messages in a cache
   list and the ID of the packet in a cache history stack. When the flow is released,
   the first node from the cache history list is retrieved from the actual cache data
   structure.

### Implementation of CACHE Management Unit (CMU)

Cache history list is a list based on FIFO principle that contains every new message
>>>>>>> 77cf785 (update readme)
encountered. When a message needs to be returned, the cache history returns the first
client ID which it will be search for in the cache hash table. The collision inside
the table are resolved using double-linked list for efficiency.
These operations will syncronize multiple threads considering any message may operate
in this area at any time.

<!--#### Security Concerns ->
Data stored in the Cache must be protected from malicious actors.
Every instance from the cache will be encrypted using AES encryption using
CBC (cipher block chaining).-->

4. Connecting to the GUI app -> using a TCP socket for managing connectivity between
   the main proxy server and the client side application.

5. Forwarding the messages -> every message may be forwarded to the actual destination
   server or to the initial source.

### Internal functionalities

<!-- - Process Listener -
The listening operations are undertaken by separate processes derived from the main process.
This type of processes are named sibling processes because they act as collaborators for the
parent process.
- Child Process Handling -
  When a message is received by the listener process, it creates a child process for managing
  the resource independently. This approach is useful for execution isolation of the workflow
  of that resource. The child process thread will act based on the condition variables used to
  detect the GUI environment necessities (if it needs more packets for rendering or not).
  If the environment variable for interception is off, then the proxy server will redirect the
  message as usual to the destination server creating its own socket with the server and
  transmitting the resource.
  If any packets are not required by the client app, then the child process thread will encrypt
  the message and store it in the cache management unit.
- Process for GUI Commands -
  While those processes manages the request, another process is listening for commands from the
  GUI app. After receiving a specific command alongside the message argument, the process will
  decide what to do next: drop the message or forward it using a TCP connection.
  OBS. We considerate that the GUI will send a command at a time, and the user must wait until it
  executes entirely before tapping a new one.
- Response Forwarder Process -
  This process is derived from the main listener process and will act as a sibling process that
  intercepts the incoming messages from another servers and create new child processes for each
  response.
- Response Child Process - (this may be changed in the future)
  Child processes, derived from the process above, will decide as the Child Process Handling wether
  or not to transmit the information or not. -->

The proxy server uses multithreading to handle multiple client requests simultaneously, enabling it to act as an intermediary between the client and the destination server.
Each client connection is managed in a separate thread, ensuring that multiple requests can be processed concurrently without blocking or delaying other requests.

- Server Setup

The proxy server listens for incoming client connections on a specified port.
Upon accepting a new client connection, a new thread is created to handle that client.
The runConnection() function sets up the server, creating a socket, binding it, and listening for incoming client connections.
For each accepted client connection, a new thread is created. This thread is responsible for handling the communication between the client and the proxy server, ensuring that each client is processed independently.

- Request Handling

The handle_client() function is the main function executed by each thread. It handles the HTTP or HTTPS request received from the client.
If the request is an HTTP request, the server parses the request headers and forwards it to the appropriate destination server using the resolve_HTTP() function on port 80.
If the request is an HTTPS request (determined by a CONNECT method), the server establishes a tunnel between the client and the destination server using the resolve_HTTPS() function on port 443. By using HTTPS, the communication between both parts continues in an encrypted manner.

- Thread-Specific Functions

For each client request, data is read, and the request is processed. Depending on whether the request is HTTP or HTTPS, the server uses the corresponding function to resolve the destination and handle communication with the target server.
resolve_HTTP() and resolve_HTTPS() are the two key functions that manage the logic for HTTP and HTTPS requests.

HTTP Requests: The server resolves the target IP, connects to the destination server on port 80, forwards the request, and relays the response back to the client.
HTTPS Requests: For HTTPS, the server uses the CONNECT method to establish a tunnel to the destination server, allowing secure communication between the client and the server.

- Connection Handling with EPOLL

For both HTTP and HTTPS requests, the server uses epoll to monitor multiple file descriptors (sockets) simultaneously. This allows it to efficiently handle multiple connections, ensuring that the proxy server can forward data between the client and the destination server without blocking.

## GUI Functionalities

GUI application will be divided into two main sections:

- intercept: this will have 3 buttons as follows:
    - intercept on/off - for turning on/off the user interaction with the proxy server
    - forward - to forward the packet to its destination (sever or source browser)
    - drop - to drop the actual packet
<!--
The interface render a specific number of messages and when the user interactively tap
on a message, it will appear in the corresponding column as a request or response message.
The message will be visualized according to its components, also in a single column.
- ***HTTP*** history - same interface as for intercept section, but, this time, when a user
select a message, it will appear the corresponding request and response for it in the two
columns.
-->

Implementation: Python/C/C++/C# (in progress)

- intercept: this will have 3 buttons as follows: - intercept on/off - for turning on/off the user interaction with the proxy server - forward - to forward the packet to its destination (sever or source browser) - drop - to drop the actual packet
  The interface render a specific number of messages and when the user interactively tap
  on a message, it will appear in the corresponding column as a request or response message.
  The message will be visualized according to its components, also in a single column.
- **_HTTP_** history - same interface as for intercept section, but, this time, when a user
  select a message, it will appear the corresponding request and response for it in the two
  columns.  
  Implementation: Python/C/C++/C# (in progress)

## Usage

## Contributors
