/*
 * <p>Title: hello-ntsock.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */

// #include "socket.h"
// #include "nts_api.h"

#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
// #include <unistd.h>

#define h_addr h_addr_list[0] /* for backward compatibility */

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char * argv[]) {
	printf("Hello libnts client app!\n");

	// int sockfd;
	int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

	char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

	sprintf(buffer, "I am message from client");
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         error("ERROR writing to socket");
	bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");

	printf("Bye, libnts app.\n");

	return 0;
}
