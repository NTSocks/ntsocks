/*
 * <p>Title: hello-ntsock-server.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Wenxiong Zou
 * @date Jan 12, 2020 
 * @version 1.0
 */

// #include "socket.h"
// #include "nts_api.h"

#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
// #include <unistd.h>


int main(int argc, char * argv[]) {
	printf("Hello libnts server app!\n");
    
    if(argc < 2){
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[256];
	// int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
        error("ERROR opening socket");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     int n;
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s\n",buffer);
    //  n = write(newsockfd,"I got your message",18);
    //  if (n < 0) error("ERROR writing to socket");

	printf("Bye, libnts app.\n");

	return 0;
}
