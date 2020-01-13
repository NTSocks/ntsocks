/*
 * <p>Title: hello-nts-client.c</p>
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
#include <string.h>
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
        return -1;
	}


	int retval;
	socklen_t server_saddrlen;
	struct sockaddr_in server_saddr;
	server_saddr.sin_family = AF_INET;
	server_saddr.sin_port = htons(PORT);
	server_saddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    // bzero(&(server_saddr.sin_zero), sizeof(server_saddr.sin_zero));
	server_saddrlen = sizeof(server_saddr);

    retval = connect(sockfd, (struct sockaddr *)&server_saddr, server_saddrlen);
    if (retval == -1) {
        printf("connect() failed.\n");
        return -1;
    } else {
        printf("connect() success\n");
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