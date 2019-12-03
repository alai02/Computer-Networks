CC = gcc
CFLAGS = -Wall -g

all: client server addrDemo

client: UDPclient.c helpers.o
	$(CC) $(CFLAGS) -o client UDPclient.c helpers.o
	
server: UDPserver.c helpers.o
	$(CC) $(CFLAGS) -o server UDPserver.c helpers.o

addrDemo: getaddrinfoDemo.c
	$(CC) $(CFLAGS) -o addrDemo getaddrinfoDemo.c

helpers.o: networkHelpers.c 
	$(CC) $(CFLAGS) -c networkHelpers.c -o helpers.o

clean:
	rm *.o client server addrDemo