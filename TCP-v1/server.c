#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#define PORTNUM "12045"
#define MAXRCVLEN 4096
#define MAXBUFFERLEN 20000
#define FILEDIR "server_files/"

int file_size;	// number of chunks of data the client will send
int buffer_size; // size of each message sent
char filename[100];
int my_socket;            // socket used to listen for incoming connections

void handleError(int con_socket, FILE* f_ptr, char* error_message) {
	fprintf(stderr, "%s", error_message);
	if (con_socket != -1) {
		close(con_socket);
	}
	if (f_ptr != NULL) {
		fclose(f_ptr);
	}
}

int validateHeader(char* header_info) {

	char* token = strtok(header_info, "-");
	if (token == NULL) {
		return 1;
	}
	file_size = atoi(token);
	if (file_size < 1) {
		return 1;
	}
	token = strtok(NULL, "-");
	if (token == NULL) {
		return 1;
	}
	buffer_size = atoi(token);
	if (buffer_size < 1 || buffer_size > MAXBUFFERLEN) {
		return 1;
	}
	token = strtok(NULL, "-");
	if (token == NULL) {
		return 1;
	}
	strcpy(filename, token);

	return 0;
}

// Close the socket if user quits the program using ctrl-c
void INThandler(int sig) {
	signal(sig, SIG_IGN);
	close(my_socket);
	exit(0);
}

int main(int argc, char *argv[])
{
	// Set a signal when server starts
	signal(SIGINT, INThandler);

	struct sockaddr_in dest; // socket info about the machine connecting to us
	// struct sockaddr_in serv; // socket info about our server
	socklen_t socksize = sizeof(struct sockaddr_in);

	//Socket Variables
	struct addrinfo hints, *servinfo, *listenAddr;
	//Set up the parameters for getaddrinfo call
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	//Get address information about the host that the server runs on
	int return_value;
	if ((return_value = getaddrinfo(NULL, PORTNUM, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(return_value));
		return 1;
	}

	int iSetOption = 1;
	// loop through all the results and bind to the first we can
	for(listenAddr = servinfo; listenAddr != NULL; listenAddr = listenAddr->ai_next) {
		if ((my_socket = socket(listenAddr->ai_family, listenAddr->ai_socktype, listenAddr->ai_protocol)) == -1) {
			fprintf(stderr, "server: Unable to connect to socket.\n");
			continue;
		}
		setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
		if (bind(my_socket, listenAddr->ai_addr, listenAddr->ai_addrlen) == -1) {
			close(my_socket);
			printf("Error code: %d\n", errno);
			fprintf(stderr, "server: Unable to bind to socket.\n");
			continue;
		}
		break;
	}

	// start listening, allowing a queue of up to 1 pending connection
	listen(my_socket, 10);

	// Create a socket to communicate with the client that just connected
	int con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);

	char buffer[MAXRCVLEN + 1]; // +1 so we can add null terminator

	while(con_socket)
	{
		printf("\nIncoming connection from: %s \n", inet_ntoa(dest.sin_addr));

		int bytes_transmitted = recv(con_socket, buffer, MAXRCVLEN , 0);
		if (bytes_transmitted < 0) {
			handleError(con_socket, NULL, "Failed to accept client request\n");
			con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);
			continue;
		}
		buffer[bytes_transmitted] = '\0';
		if (validateHeader(buffer) == 1) {
			handleError(con_socket,NULL, "Failed, invalid client header\n");
			con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);
			continue;
		}

		//open file for output
		char filepath[100] = "";
		strcat(filepath, FILEDIR);
		strcat(filepath, filename);
		FILE* f_ptr = fopen(filepath, "w+");

		if (f_ptr == NULL) {
			fprintf(stderr, "Failed to open file");
			con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);
			continue;
		}

		// acknowledge client request
		char ready_message[4] = "200";
		bytes_transmitted = send(con_socket, ready_message, strlen(ready_message), 0);
		if (bytes_transmitted != strlen(ready_message)) {
			handleError(con_socket,f_ptr, "Failed to acknowledge client request");
			con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);
			continue;
		}

		char chunk_buffer[buffer_size +1];
		int message_success = 0;
		int total_bytes = 0;
		while(total_bytes < file_size){
			bytes_transmitted = recv(con_socket, chunk_buffer, buffer_size, 0);
			total_bytes += bytes_transmitted;

			if (bytes_transmitted < 0) {
				message_success = 1;
				break;
			}
			if (bytes_transmitted > 0) {
				chunk_buffer[bytes_transmitted] = '\0';
				printf("%s", chunk_buffer);
				fprintf(f_ptr, "%s", chunk_buffer);
			}
		}
		fclose(f_ptr);
		printf("\n");

		if (message_success == 1) {
			handleError(con_socket,f_ptr, "Failed to accept message");
			con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);
			continue;
		}

		char complete_message[4] = "201";
		bytes_transmitted = send(con_socket, complete_message, strlen(complete_message), 0);
		if (bytes_transmitted != strlen(complete_message)) {
			handleError(con_socket,f_ptr, "Failed to confirm client request");
			con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);
			continue;
		}
		//Close current connection
		close(con_socket);

		//Continue listening for incoming connections
		con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize);
	}

	close(my_socket);
	return EXIT_SUCCESS;
}
