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
 
#define PORTNUM 2300
#define MAXRCVLEN 500
 
int main(int argc, char *argv[])
{
	int len;
	char buffer[MAXRCVLEN + 1]; // +1 so we can add null terminator
  
	struct sockaddr_in dest; // socket info about the machine connecting to us
	struct sockaddr_in serv; // socket info about our server
	int mysocket;            // socket used to listen for incoming connections
	socklen_t socksize = sizeof(struct sockaddr_in);

	memset(&serv, 0, sizeof(serv));           // zero the struct before filling the fields
	
	serv.sin_family = AF_INET;                // Use the IPv4 address family
	serv.sin_addr.s_addr = htonl(INADDR_ANY); // Set our address to any interface 
	serv.sin_port = htons(PORTNUM);           // Set the server port number 

	/* Create a socket.
   	   The arguments indicate that this is an IPv4, TCP socket
	*/
	mysocket = socket(AF_INET, SOCK_STREAM, 0);
  
	// bind serv information to mysocket
	// Unlike all other function calls in this example, this call to bind()
	//does some basic error handling
	if (bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) != 0){
		printf("Unable to open TCP socket on localhost:%d\n", PORTNUM);

	// 	//strerror() returns a string representation of the system variable "errno"
	// 	//errno is the integer code of the error that occured during the last system call from this process
	// 	//need to include errno.h to use this function
		printf("%s\n", strerror(errno));
		close(mysocket);
		return 0;
	}

	// start listening, allowing a queue of up to 1 pending connection
	listen(mysocket, 1);
	
	// Create a socket to communicate with the client that just connected
	int consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
  
	while(consocket)
	{
		printf("Incoming connection from %s \n", inet_ntoa(dest.sin_addr));
		
		// Receive data from the client
		len = recv(consocket, buffer, MAXRCVLEN, 0);
		buffer[len] = '\0';
		printf("Received %s\n", buffer);
		
		//Convert data to upper case
		int bufLen = strlen(buffer);
		for(int i = 0; i < bufLen; i++){
			buffer[i] = toupper(buffer[i]);
		}
		
		//Send data to client
		send(consocket, buffer, strlen(buffer), 0); 
		
		//Close current connection
		close(consocket);
		
		//Continue listening for incoming connections
		consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
	}

	close(mysocket);
	return EXIT_SUCCESS;
}