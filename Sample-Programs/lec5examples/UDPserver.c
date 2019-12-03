/*
 ** UDPserver.c -- a datagram upper-case echo server
 Based on the UDP socket demo in Beej's Guide to Network Programming
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "networkStuff.h"

#define MYPORT "12000"    // the port users will be connecting to

#define MAXBUFLEN 100


int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *listenAddr;
    int rv;
    int numbytes;
    struct sockaddr_storage clientAddr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char ipAddr[INET6_ADDRSTRLEN];
    
    //Set up the parameters for getaddrinfo call
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    //Get address information about the host that the server runs on
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and bind to the first we can
    for(listenAddr = servinfo; listenAddr != NULL; listenAddr = listenAddr->ai_next) {
        if ((sockfd = socket(listenAddr->ai_family, listenAddr->ai_socktype, listenAddr->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        
        if (bind(sockfd, listenAddr->ai_addr, listenAddr->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        
        break;
    }
    
    if (listenAddr == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }
    
    freeaddrinfo(servinfo);
    
    printf("server: waiting to recvfrom...\n");
    //Receive the string from the client
    addr_len = sizeof(clientAddr);
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&clientAddr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    
    //Get IP adress of the client as a string and put in the variable ipAddr
    inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), ipAddr, sizeof ipAddr);
    printf("server: got packet from %s\n", ipAddr);
    printf("server: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("server: packet contains \"%s\"\n", buf);
    
    //Convert message to upper case
    for (int i = 0; i < numbytes; i++){
        buf[i] = toupper(buf[i]);
    }
    
    //Send converted message back to client
    if ((numbytes = sendto(sockfd, buf, numbytes, 0, (struct sockaddr *)&clientAddr, addr_len)) == -1) {
        perror("client: sendto");
        exit(1);
    }
    
    //Get IP adress of the client as a string and put in the variable ipAddr
    inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), ipAddr, sizeof ipAddr);
    printf("Server sent %d bytes containg \"%s\" to %s\n", numbytes, buf, ipAddr);
    
    close(sockfd);
    
    return 0;
}
