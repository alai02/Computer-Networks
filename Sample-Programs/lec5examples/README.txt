This example contains a getaddrinfo() demo, as well as a UDP client/server demo

Type "make all" to compile all examples
Type "make clean" to clear all comppiled files

To run the getaddrinfo() example, addrDemo, type: "addrDemo hostname service", where "service" is either a network service, e.g. "http", ort a port number, e.g. "80"

To run he UDP cliner/server example:
1. Start the server on the desired host
2. Run the client top connect to the desired host:
- Running "client" tries to send a message to localhost on port 12000
- Running "client host port" tries to send a message to the desired host:port address
