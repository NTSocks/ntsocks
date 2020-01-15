/*
 * <p>Title: hello-nts.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */


// #include "nts_api.h"

#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// #include <unistd.h>

// #include "socket.h"
#define SERVER_IP "10.10.88.202"
#define PORT 80


int main(int argc, char * argv[]) {
	printf("Hello libnts app!\n");

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd > 0)
	{
		printf("sockfd=%d \n", sockfd);
		printf("socket() success\n");
	} else {
		printf("socket() failed.\n");
	}


	int retval;
	socklen_t saddrlen;
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	saddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	saddrlen = sizeof(saddr);
	retval = bind(sockfd, (struct sockaddr*)&saddr, saddrlen);
	if (retval == -1) {
		printf("bind() failed.\n");
		return -1;
	} else {
		printf("bind() success\n");
	}

	retval = listen(sockfd, 8);
	if (retval == -1) {
		printf("listen() failed\n");
		return -1;
	} else {
		printf("listen() success\n");
	}


	int client_sockfd;
	socklen_t client_saddrlen;
	struct sockaddr_in client_saddr;
	while (1)
	{
		client_sockfd = accept(sockfd, (struct sockaddr*)&client_saddr, &client_saddrlen);
		if (client_sockfd == -1)
		{
			printf("accept() failed\n");
			return -1;
		} else {
			printf("accept() success with client nt_socket sockfd=%d\n", client_sockfd);
		}
	
	}
	

	

    
	getchar();

	// close(sockfd);

	// print_socket();
	// test_ntm_shm();
	// test_nts_shm();

	// nts_init(NTS_CONFIG_FILE);
	// getchar();
	// nts_destroy();

	printf("Bye, libnts app.\n");

	return 0;
}