#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#define DEFAULTBUFFERLEN 4096
#define MAXRCVLEN 500
#define MAXBUFFERLEN 20000
#define MAXPORTNUM 65535
#define MAXFILENAMELEN 20

//Global Variables
char *server_port_str = NULL;
char *hostname = NULL;
char *file_path = NULL;
long long buffer_size; //size of each chunk sent to the server
long long file_size; //size of the file being sent to the server

void handleError(FILE* file_pointer, int my_socket, char* msg, struct addrinfo *servinfo) {
	fprintf(stderr, "%s", msg);
	if (file_pointer != NULL) {
		fclose(file_pointer);
	}
	if (my_socket != -1) {
		close(my_socket);
	}
	free(server_port_str);
	free(hostname);
	freeaddrinfo(servinfo);
}

char* extractFilename() {
	char *last = strrchr(file_path, '/');
	if (last != NULL) {
		return last +1;
	}
	return file_path;
}

void printUsage(){
	printf("Usage: ./client server-IP-address:port-number -f <filepath> [-b <int>]\n");
	printf("   -f path to the file to send to the server.");
	printf("   -b Optional parameter that can be used to set the buffer\n");
	printf("      length for client messages sent to the sever. Must be an integer value.\n");
}

void getArgs(int argc, char* argv[]) {
	if (argc != 4 && argc != 6) {
		fprintf(stderr, "%s", "Invalid argument count.\n");
		printUsage();
		exit(EXIT_FAILURE);
	}
	//Get the server IP address and port
	char *addr_port_str = strdup(argv[1]);
	char* token = strtok(addr_port_str, ":");
	if (token == NULL) {
		fprintf(stderr, "%s", "Invalid hostname");
		printUsage();
		exit(EXIT_FAILURE);
	}
	hostname = malloc(sizeof(char) * strlen(token) +1);
	strncpy(hostname, token, strlen(token));
	token = strtok(NULL, ":");
	if (token == NULL) {
		fprintf(stderr, "%s", "Invalid port number");
		printUsage();
		exit(EXIT_FAILURE);
	}
	server_port_str = malloc(sizeof(char) * strlen(token) +1);
	strncpy(server_port_str, token, strlen(token));
	free(addr_port_str);

	//check that both server address and port were given
	if (hostname == NULL || server_port_str == NULL) {
		char error_str[100];
		sprintf(error_str, "%s\n%s", "Error: Invalid server address and port given: ", argv[1]);
		fprintf(stderr, "%s", error_str);
		printUsage();
		exit(EXIT_FAILURE);
	}

	int server_port = -1;
	server_port = atoi(server_port_str);
	if (server_port < 1 || server_port > MAXPORTNUM) {
		fprintf(stderr, "%s", "Invalid port number. Please use a valid number between 1 and 65535\n");
		exit(EXIT_FAILURE);
	}

	//Optional parameter for the length of the buffer
	buffer_size = -1;
	if (argc > 3) { //possibly contains buffer flag
		for (int i = 2; i < argc; i=i+2) {
			if (strcmp(argv[i], "-b") == 0) {
				buffer_size = atoi(argv[i+1]);
				if (buffer_size < 1 || buffer_size > MAXBUFFERLEN) {
					fprintf(stderr, "%s", "Invalid buffer length given.\n");
					printUsage();
					exit(EXIT_FAILURE);
				}
			}
			if (strcmp(argv[i], "-f") == 0) {
				file_path = argv[i+1];
			}
		}
	}

	if (file_path == NULL) {
		fprintf(stderr, "%s", "Error - No filepath given\n");
		printUsage();
		exit(EXIT_FAILURE);
	}

	if (buffer_size == -1) {
		buffer_size = DEFAULTBUFFERLEN; //set to default
	}

}

int main(int argc, char *argv[])
{
	getArgs(argc, argv);
	char* file_name = extractFilename();

	if (strlen(file_name) > 20) {
		printf("Error - The filename must be 20 characters or less.");
		exit(1);
	}

	// Open the file
	FILE* file_pointer = fopen(file_path, "rb");
	if (file_pointer == NULL) {
		fprintf(stderr, "%s", "Failed to open file");
		return EXIT_FAILURE;
	}

	//Socket Related Variables
	struct addrinfo hints, *servinfo, *destAddr;
	int my_socket;

	//Initialize info for getaddr
	memset(&hints, 0, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	//Resolve host name
	int returnval;
	if ((returnval = getaddrinfo(hostname, server_port_str, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(returnval));
		handleError(file_pointer, -1, "Failed to get address info", servinfo);
		return EXIT_FAILURE;
	}

	// loop through all the results and make a socket
	for(destAddr = servinfo; destAddr != NULL; destAddr = destAddr->ai_next) {
		if ((my_socket = socket(destAddr->ai_family, destAddr->ai_socktype, destAddr->ai_protocol)) == -1) {
			fprintf(stderr, "%s", "client: socket");
			continue;
		}
		break;
	}

	if (destAddr == NULL) {
		handleError(file_pointer, -1, "Error: failed to create socket\n", servinfo);
		return EXIT_FAILURE;
	}

	// Connect to the server
	if (connect(my_socket,destAddr->ai_addr,destAddr->ai_addrlen)==-1) {
		handleError(file_pointer, -1, "Error: Failed to connect to socket.\n", servinfo);
		return EXIT_FAILURE;
	}

	// Calculate length of the file
	fseek(file_pointer, 0, SEEK_END); //go to the end of the file
	file_size = ftell(file_pointer); //get length of file
	rewind(file_pointer); //go back to beginning of file

	char buffer[MAXRCVLEN +1];
	char message_chunk[buffer_size +1];


	char header_message[36];
	snprintf(header_message, 36, "%lld-%lld-%s", file_size, buffer_size, file_name);

	int bytes_transmitted = send(my_socket, header_message, strlen(header_message), 0);
	if (bytes_transmitted != strlen(header_message)) {
		handleError(file_pointer, my_socket, "Failed to send header information, closing socket connection\n", servinfo);
		return EXIT_FAILURE;
	}

	bytes_transmitted = recv(my_socket, buffer, MAXRCVLEN, 0);
	if (bytes_transmitted < 0) {
		handleError(file_pointer, my_socket, "Failed, server could not acknowledge request\n", servinfo);
		return EXIT_FAILURE;
	} else if (bytes_transmitted == 0) {
		handleError(file_pointer, my_socket, "Failed, server did not accept the connection\n", servinfo);
		return EXIT_FAILURE;
	}
	buffer[bytes_transmitted] = '\0';

	if (strcmp(buffer, "402") == 0) {
		handleError(file_pointer, my_socket, "Failed, duplicate file name\n", servinfo);
		return EXIT_FAILURE;
	} else if (strcmp(buffer, "200") != 0) {
		printf("Respose: %s", buffer);
		handleError(file_pointer, my_socket, "Failed, invalid header sent\n", servinfo);
		return EXIT_FAILURE;
	}

	double percent_sent = 0.0;

	while(!feof(file_pointer)){
		memset(message_chunk, 0, sizeof(message_chunk));
		int bytes_read = fread(message_chunk, sizeof(char), buffer_size, file_pointer);
		if (bytes_read > 0){
			message_chunk[bytes_read] = '\0';
			bytes_transmitted = send(my_socket, message_chunk, strlen(message_chunk), 0);
			if (bytes_transmitted != strlen(message_chunk)) {
				handleError(file_pointer, my_socket, "Failed to send message chunk\n", servinfo);
				return EXIT_FAILURE;
			}
			// Sleep for one second after every 10% of the file is sent
			percent_sent += bytes_transmitted;
			if (percent_sent / (double)file_size > 0.1) {
				sleep(1);
				percent_sent = 0.0;
			}
		}
	}

	bytes_transmitted = recv(my_socket, buffer, MAXRCVLEN, 0);
	if (bytes_transmitted < 0) {
		handleError(file_pointer, my_socket, "Failed, server not responsive\n", servinfo);
		return EXIT_FAILURE;
	}
	buffer[bytes_transmitted] = '\0';
	if (strcmp(buffer, "201") != 0) {
		handleError(file_pointer, my_socket, "Failed, server could not process message\n", servinfo);
		return EXIT_FAILURE;
	}

	handleError(file_pointer, my_socket, "Message sent\n", servinfo);
	return EXIT_SUCCESS;
}
