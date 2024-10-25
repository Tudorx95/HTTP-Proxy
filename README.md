
# HTTP Proxy App (Proxy Eye)
Proxy Eye is a C based application designed to manage all network traffic between a web browser and destination 
servers. Users can interact behind the browser resources via a desktop application, giving insights about the 
specific assets required to render a website correctly.

## Table of Contents
- [What is an open proxy server?](#project_overview)
- [Project Construction Diagram](#project_construction)
- [Proxy Server Functionalities](#server_app)
- [GUI Functionalities](#gui_app)
- [Usage](#run_app)
- [Contributors](#contributors)

### What is an open proxy server?
A proxy server is a specific type of server application used to forward traffic between the web browser 
and the actual destination server. In an open ***HTTP*** proxy server, the resources are managed by 
independent clients and it is distributed, or not, according to their actions. 

![Principles of a proxy server - Wikipedia](https://upload.wikimedia.org/wikipedia/commons/b/bb/Proxy_concept_en.svg)

The Proxy Eye application is intended for educational purposes, representing the ***HTTP*** messages in 
their pure form, alongside all its specifications.

### Project Construction Diagram 

![Proxy Server Connections](Proxy\ Diagram.jpg)

### Proxy Server Functionalities

1. TCP Socket -> Packets from web browser are captured via a TCP socket 
2. Registration of packets -> using a cache list data structure and a cache history list.
The data structure is necessarily for managing the packet flow from the listener to
the GUI application. 
When there are enough packets send, the server will store those messages in a cache
list and the ID of the packet in a cache history stack. When the flow is released, 
the first node from the cache history list is retrieved from the actual cache data
structure. 

#### Implementation of CACHE 
Cache history list is a list based on FIFO principle that contains every new message 
encountered. When a message needs to be returned, the cache history returns the first
client ID which it will be search for in the cache hash table. The collision inside 
the table are resolved using double-linked list for efficiency.

3. Security Concerns -> 

