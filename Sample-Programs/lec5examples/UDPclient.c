/*
 ** UDPclient.c -- a datagram client
 Based on the UDP socket demo in Beej's Guide to Network Programming
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "networkStuff.h"

#define SERVERPORT "12000"    // the port users will be connecting to
#define MAXBUFLEN 100
#define MSGLEN 1000

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *destAddr;
    int rv;
    int numbytes;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char hostName[100] = "localhost";
    char message[MSGLEN] = "I am a message, hear me roar";
    char ipAddr[INET6_ADDRSTRLEN];
    
    if (argc != 1 && argc != 3){
        printf("Usage: %s mst have 0 or 2 arguments\n", argv[0]);
        printf("\"%s\" sends a default string to local host\n", argv[0]);
        printf("\"%s host_name string\" sends the argument string to local host\n", argv[0]);
        return 0;
    }
    //Initialize the destination host and message
    if (argc == 3) {
        if (strlen(argv[1]) > 99){
            fprintf(stderr, "host name must be less than 100 characters long\n");
            exit(1);
        }else{
            strcpy(hostName, argv[1]);
        }
        
        if (strlen(argv[2]) > MSGLEN-1){
            fprintf(stderr, "message must be less than 1000 characters long\n");
            exit(1);
        }else{
            strcpy(message, argv[2]);
        }
    }
    
    //Initialize infor for getaddr
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    //Resolve host name
    if ((rv = getaddrinfo(hostName, SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and make a socket
    for(destAddr = servinfo; destAddr != NULL; destAddr = destAddr->ai_next) {
        if ((sockfd = socket(destAddr->ai_family, destAddr->ai_socktype, destAddr->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        break;
    }
    
    if (destAddr == NULL) {
        fprintf(stderr, "client: failed to create socket\n");
        return 2;
    }
    
    //Send the message to the server.  Remember, the adress destAddr was returned by getaddrinfo and
    //has all the information for connection to the host that you specified as a commnd line argument
    if ((numbytes = sendto(sockfd, message, strlen(message), 0, destAddr->ai_addr, destAddr->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }

    //Get IP adress of the server as a string and put in the variable ipAddr
    
    struct sockaddr* tmpAddr = (struct sockaddr *)destAddr->ai_addr;
    struct sockaddr_in* tmpAddrIn;

    switch (tmpAddr->sa_family){ 
        case AF_INET:
            tmpAddrIn = (struct sockaddr_in *) tmpAddr;
            printf("Client: sent %d bytes to %s using IPv4\n", numbytes, inet_ntoa(tmpAddrIn->sin_addr));
            break;
        case AF_INET6:
            inet_ntop(destAddr->ai_family, get_in_addr((struct sockaddr *)destAddr->ai_addr), ipAddr, sizeof ipAddr);
            printf("Client: sent %d bytes to %s using IPv6\n", numbytes, ipAddr);
            break;
        default:
            inet_ntop(destAddr->ai_family, get_in_addr((struct sockaddr *)destAddr->ai_addr), ipAddr, sizeof ipAddr);
            printf("Client: sent %d bytes to %s using a magic address\n", numbytes, ipAddr);
            break;
    }
    
    

    //Receive the upper-case message from server
    addr_len = sizeof(their_addr);
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    //Since the strig terminator wasn't sent, we need to remember to ternminate the char array
    buf[numbytes]='\0';
    
    //Get IP adress of the server as a string and put in the variable ipAddr
    inet_ntop(destAddr->ai_family, get_in_addr((struct sockaddr *)destAddr->ai_addr), ipAddr, sizeof ipAddr);
    printf("Received \"%s\" from %s\n", buf, ipAddr);
    
    
    freeaddrinfo(servinfo);
    
    close(sockfd);
    
    return 0;
}
