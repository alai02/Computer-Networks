CC = gcc
CFLAGS = -Wall -g

all: client server folder

client: client.c 
	$(CC) $(CFLAGS) -o client client.c
	
server: server.c 
	$(CC) $(CFLAGS) -o server server.c 

folder: 
	mkdir -p server_files

clean:
	rm client server server_files/*.txt