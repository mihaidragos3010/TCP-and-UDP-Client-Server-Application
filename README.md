# TCP-and-UDP-Client-Server-Application
The goal of the project was to use TCP and UDP protocols through sockets.

In implementing this project, I had to create the functionality of a server and a client that communicate via TCP. The server also receives a set of UDP messages as topics, which are converted into TCP messages sent to clients.

Steps of implementation:

SERVER:
--
- Initialized main TCP and UDP sockets for client connections.
- Removed the Nagle algorithm from each TCP port.
- Constructed a vector 'poll' to listen on multiple sockets simultaneously.
- Defined the structures 'udp_message' and 'tcp_message' for communication between clients and the server.
- Used the 'server_messages' structure to send commands from clients to the server over TCP.
- Used the 'subscriber' structure to store various attributes of each client.
- Each subscriber contains a vector of 'topic' structures representing the topics to which they have subscribed and whether they respect the 'sf' property. Each element in this vector of topics contains another vector of 'tcp_message' structures, representing the concrete storage of messages for the subscribed topics.

SUBSCRIBER:
--
- Initialized 2 sockets, one for stdin and the other for the TCP socket used to send messages to the server.
- One socket receives commands from the keyboard and executes them.
- Another socket receives messages from the server over TCP and checks if it received a command to close or to display the topic it received.
- I use a vector of subscribers to save each client and the necessary parameters. Each subscriber contains a vector of topics representing the topics they have subscribed to and whether they have used the 'sf' property. In this vector, only a template of the topic is saved, not the specific message. Each topic structure contains another vector of 'tcp_messages' that saves the concrete TCP messages to be sent to the client upon reconnection.
- Program limitations: The program can support a maximum of 10 clients and a maximum of 31 topics for each topic model with the 'sf' option.

Note: I noticed that when running the checker multiple times, the 'quick_flow' test fails. After recompiling with 'make clean' and retesting, it should work correctly.
