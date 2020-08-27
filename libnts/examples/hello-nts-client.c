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
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

// #include "socket.h"


#define SERVER_IP "10.10.88.210"
#define PORT 80

#define test_msg "Hello, NTB World!"

int main(int argc, char * argv[]) {

	if (argc < 3) {
		printf("Usage: %s <Server IP> <Port> \n", argv[0]);
		return 0;
	}

	int port = atoi(argv[2]);
	char * server_ip = argv[1];

	printf("Hello libnts app!\n");

	int client_sockfd;
	client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_sockfd > 0)
	{
		printf("client_sockfd=%d \n", client_sockfd);
		printf("socket() success\n");
	} else {
		printf("socket() failed.\n");
        return -1;
	}


	int retval;
	socklen_t server_saddrlen;
	struct sockaddr_in server_saddr;
	server_saddr.sin_family = AF_INET;
	server_saddr.sin_port = htons(port);
	server_saddr.sin_addr.s_addr = inet_addr(server_ip);
    // bzero(&(server_saddr.sin_zero), sizeof(server_saddr.sin_zero));
	server_saddrlen = sizeof(server_saddr);

    retval = connect(client_sockfd, (struct sockaddr *)&server_saddr, server_saddrlen);
    if (retval == -1) {
        printf("connect() failed.\n");
        return -1;
    } else {
        printf("connect() success\n");
    }


	// test for write payload that bytes length > 256
	char recv_data[512];
	memset(recv_data, 0, sizeof(recv_data));
	size_t recv_bytes_len;
	recv_bytes_len = read(client_sockfd, recv_data, 512);
	if (recv_bytes_len > 0) {
		printf("read large message [%d bytes] success \n", (int)recv_bytes_len);
		for (size_t i = 0; i < recv_bytes_len; i++)
		{
			printf("%c", recv_data[i]);
		}
		printf("\n");
		
	}


	// test for write first in server
	// char recv_msg[32] = {0};
	// size_t read_msg_len;
	// read_msg_len = read(client_sockfd, recv_msg, sizeof(recv_msg));
	// if (read_msg_len > 0) {
	// 	printf("read() success with recv msg = '%s' \n", recv_msg);
	// }
	
	// size_t write_msg_len;
	// write_msg_len = write(client_sockfd, test_msg, strlen(test_msg));
	// if (write_msg_len > 0) {
	// 	printf("write() success with send msg = '%s'\n", test_msg);
	// }
    
	close(client_sockfd);

	getchar();

	// close(client_sockfd);

	// print_socket();
	// test_ntm_shm();
	// test_nts_shm();

	// nts_init(NTS_CONFIG_FILE);
	// getchar();
	// nts_destroy();

	printf("Bye, libnts app.\n");

	return 0;
}