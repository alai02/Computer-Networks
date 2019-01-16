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
#include <pthread.h>

#define PORTNUM "12045"
#define MAXRCVLEN 4096
#define MAXBUFFERLEN 20000
#define FILEDIR "server_files/"

int my_socket;		// socket used to listen for incoming connections
int accept_new;

typedef struct record_node {
    pthread_t thread_id;
	int socket_id;
	char file_name[21];
    struct record_node* next;
} RecordNode; // Record node to store thread specific record information

typedef struct {
	int size;
    RecordNode* head;
    RecordNode* tail;
    pthread_mutex_t mutex; 
    pthread_cond_t cond;
} RecordQueue;	// Singly linked list with a mutex and condition variable

RecordQueue* q; // Global record queue

// Close the socket if user quits the program using ctrl-c
void INThandler(int sig) {
	signal(sig, SIG_IGN);
	close(my_socket);
	exit(0);
}

// Complete the active transfers and close the socket when user quits the program from the user menu
void completeActiveTransfers(int signal) {

	printf("Shutting down the server\n");
	printf("Enter h for hard shutdown (Close any active connections)\n");
	printf("Enter any key for soft shutdown (Allow active connection to complete transfer) \n");
	char input_char;
	scanf(" %c", &input_char);

	if (input_char != 'h') {

		int i = 0;
		pthread_mutex_lock(&q->mutex);

		int queue_size = q->size;
		if (queue_size > 0) {
			printf("Completing %d active transfers\n", queue_size);

			pthread_t active_threads[queue_size];
			RecordNode* current = q->head;

			while(current != NULL){
				active_threads[i++] = current->thread_id;
				current = current->next;
			}
			pthread_mutex_unlock(&q->mutex);

			for (i = 0; i < queue_size; i++) {
				int ret = pthread_join(active_threads[i], NULL);
				if (ret != 0) {
					perror("Failed to join thread\n");
				}
			}

		} else {
			pthread_mutex_unlock(&q->mutex);
			printf("No active transfers. shutting down.\n");
		}
	}

	close(my_socket);
	exit(0);
}

void handleError(int con_socket, FILE* file_ptr, char* error_message) {
	fprintf(stderr, "%s", error_message);
	if (con_socket != -1) {
		close(con_socket);
	}
	if (file_ptr != NULL) {
		fclose(file_ptr);
	}
}

int validateHeader(char* header_info, long long* file_size, long long* buffer_size, void* file_name) {
	char* token = strtok_r(header_info, "-", &header_info);
	if (token == NULL) {
		return 1;
	}
	*file_size = atoll(token);
	if (*file_size < 1) {
		return 1;
	}
	token = strtok_r(NULL, "-", &header_info);
	if (token == NULL) {
		return 1;
	}
	*buffer_size = atoll(token);
	if (*buffer_size < 1 || *buffer_size > MAXBUFFERLEN) {
		return 1;
	}
	token = strtok_r(NULL, "-", &header_info);
	if (token == NULL) {
		return 1;
	}
	strncpy((char*)file_name, token, strlen(token));

	return 0;
}

// Create a queue and initilize its mutex and condition variable
RecordQueue* createRecordQueue() {
    RecordQueue* q = (RecordQueue*)malloc(sizeof(RecordQueue));
    q->head = q->tail = NULL;
	q->size = 0;
    pthread_mutex_init(&q->mutex, NULL); //Initialize the mutex
    pthread_cond_init(&q->cond, NULL); //Initialize the condition variable
    return q;
}

// Outputs all records on the queue
void displayActiveTransfers() {

    // Critical section
    pthread_mutex_lock(&q->mutex);
    //Wait for a signal telling us that there's something on the queue
    //If we get woken up but the queue is still null, we go back to sleep

    while(q->head == NULL){
        fprintf(stderr, "Message queue is empty, going to sleep...\n");
        pthread_cond_wait(&q->cond, &q->mutex);
        fprintf(stderr, "Displaying active transfers...\n");
    }
    //By the time we get here, we know q->head is not null, so it's all good
	RecordNode* current = q->head;
	// print out active messages
    while(current != NULL){
        printf("Active transfer: %s\n", current->file_name);
		current = current->next;
    }
    //Release lock
    pthread_mutex_unlock(&q->mutex);
    return;
}

// Append active transfer to the queue
int addActiveTransfer(int socket_id, char file_name[], pthread_t tid) {

    RecordNode* node = (RecordNode*) malloc(sizeof(RecordNode));
    node->socket_id = socket_id;
	node->thread_id = tid;
	strncpy(node->file_name, file_name, strlen(file_name));
	node->file_name[strlen(file_name)] = '\0';
    node->next = NULL;
    
    pthread_mutex_lock(&q->mutex);		// critical section

	RecordNode* temp = q->head;
	while(temp != NULL) {
		if (strcmp(temp->file_name, file_name) == 0) {
			free(node);
			pthread_mutex_unlock(&q->mutex);
			return 1;
		}
		temp = temp->next;
	}

    if (q->tail != NULL) {
        q->tail->next = node;       // append after tail
        q->tail = node;
    } else {
        q->tail = q->head = node;   // first node
    }

	q->size++;	// Increase the size of the queue
    //Signal the consumer thread waiting on this condition variable
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
	return 0;
}

// Remove completed transfer from the queue
void removeCompletedTransfer(int socket_id) {

    pthread_mutex_lock(&q->mutex); 		// Critical section

	if (q->head == NULL) {
		pthread_mutex_unlock(&q->mutex);
		return;
	}

	RecordNode* temp = q->head;
	RecordNode* prev = NULL;

	if (temp != NULL && temp->socket_id == socket_id) {
		if (temp->next == NULL) {
			q->tail = q->head = NULL;
		} else {
			q->head = temp->next;
		}
		free(temp);
		q->size--;
		pthread_mutex_unlock(&q->mutex);
		return;
	}

	while (temp != NULL && temp->socket_id != socket_id) {
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL ) {
		pthread_mutex_unlock(&q->mutex);
		return;
	}

	if (temp->next == NULL) {
		q->tail = prev;
	} 
	prev->next = temp->next;
	free(temp);

    if (q->head == NULL) {
        q->tail = NULL; 				// Last node removed
    }
	q->size--; 							// Decrement size of the queue
    pthread_mutex_unlock(&q->mutex); 	//Release lock
    return;
}

// Transfers a record from a client to the server
void* transferRecord(void* arg) {

    int socket_id = *(int*)arg; 	// Socket number passed from main thread 
	char buffer[MAXRCVLEN +1];		// Buffer to read data to from the client
	long long file_size;			// Number of chunks of data the client will send
	long long buffer_size; 			// Size of each message sent
	char file_name[21];				// Name of the file being transfered

	int bytes_transmitted = recv(socket_id, buffer, MAXRCVLEN , 0);
	if (bytes_transmitted < 0) {
		handleError(socket_id, NULL, "Failed to accept client request\n");
		return NULL;
	}
	buffer[bytes_transmitted] = '\0';
	if (validateHeader(buffer, &file_size, &buffer_size, &file_name) == 1) {
		handleError(socket_id, NULL, "Failed, invalid client header\n");
		return NULL;
	}

	pthread_t self;			// Get the thread information to keep track thread
	self = pthread_self();

	int duplicate_transfer = addActiveTransfer(socket_id, file_name, self);
	if (duplicate_transfer == 1) { 		// Duplicate file name in active transfer
		char dup_message[4] = "402";
		bytes_transmitted = send(socket_id, dup_message, strlen(dup_message), 0);
		if (bytes_transmitted != strlen(dup_message)) {
			handleError(socket_id, NULL, "Failed, to notify client about duplicate message\n");
			return NULL;
		}
	}

	char filepath[50] = "";
	strncpy(filepath, FILEDIR, strlen(FILEDIR));
	strncat(filepath, file_name, strlen(file_name));

	FILE* f_ptr = fopen(filepath, "w+"); 	// Open file for output
 	if (f_ptr == NULL) {					// Check if file is open
		fprintf(stderr, "Failed to open file\n");
		return NULL;
	}

	// acknowledge client request
	char ready_message[4] = "200";
	bytes_transmitted = send(socket_id, ready_message, strlen(ready_message), 0);
	if (bytes_transmitted != strlen(ready_message)) {
		handleError(socket_id, f_ptr, "Failed to acknowledge client request\n");
		return NULL;
	}

	char chunk_buffer[buffer_size +1];
	int message_failure = 0;
	int total_bytes = 0;
	while(total_bytes < file_size){
		bytes_transmitted = recv(socket_id, chunk_buffer, buffer_size, 0);
		total_bytes += bytes_transmitted;
		if (bytes_transmitted < 0) {
			message_failure = 1;
			break;
		}
		if (bytes_transmitted > 0) {
			chunk_buffer[bytes_transmitted] = '\0';
			fprintf(f_ptr, "%s", chunk_buffer);
		}
	}
	
	if (message_failure == 1) {
		handleError(socket_id, f_ptr, "Failed to accept message \n");
		return NULL;
	}

	char complete_message[4] = "201";
	bytes_transmitted = send(socket_id, complete_message, strlen(complete_message), 0);
	if (bytes_transmitted != strlen(complete_message)) {
		handleError(socket_id, f_ptr, "Failed to confirm client request\n");
		return NULL;
	}
	
	fclose(f_ptr); 							// Close file
	close(socket_id); 						// Close connection
	removeCompletedTransfer(socket_id); 	// Remove record from transfer queue
	free(arg);								// free memory allocated to the thread
    return NULL;
}

// Transfers a record from a client to the server
void* runUserMenu() {

	int input_num;
	char c;

	printf("\nEnter 1 to display active transfers\n");
	printf("Enter 2 to shutdown the server\n");

	while(1) {
		scanf("%d", &input_num);
		if (input_num == 1) {
			displayActiveTransfers();
		} else if (input_num == 2) {
			accept_new = 1;
			raise(SIGUSR1);
			return NULL;
		} else {
			printf("Invalid input\n");
			do {
				c = getchar();
			}
			while (!isdigit(c));
			ungetc(c, stdin);
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {

	// Set a signal when server starts
	signal(SIGINT, INThandler);
	signal(SIGUSR1, completeActiveTransfers);

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
	listen(my_socket, 1);

	q = createRecordQueue(); // Initialize the record queue

	// Create a thread for the the user menu
	pthread_t uid;
	pthread_create(&uid, NULL, runUserMenu, NULL);

	int con_socket;
	accept_new = 0;

	printf("accepting connections...\n");

	while((con_socket = accept(my_socket, (struct sockaddr *)&dest, &socksize))) {

		// printf("\nIncoming connection from: %s on socket: %d\n", inet_ntoa(dest.sin_addr), con_socket);
		if (con_socket == -1 || accept_new == 1) {
			break;
		}

		pthread_t thread_id;
		int* socket_id  = malloc(sizeof(int));
		*socket_id = con_socket;

		if (pthread_create(&thread_id, NULL, transferRecord, (void*) socket_id) < 0) {
            perror("could not create thread\n");
		}

	}

	pthread_join(uid, NULL); // Wait for user menu to complete
	close(my_socket);

	return EXIT_SUCCESS;
}
