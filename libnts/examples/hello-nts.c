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
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include <unistd.h>

// #include "socket.h"
#define SERVER_IP "10.10.88.210"
#define PORT 80

#define test_msg "Hello, NTSocket Server World!"
#define large_msg "Hello,Large Msg!"


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
	// while (1)
	// {
		client_sockfd = accept(sockfd, (struct sockaddr*)&client_saddr, &client_saddrlen);
		if (client_sockfd == -1)
		{
			printf("accept() failed\n");
			return -1;
		} else {
			printf("accept() success with client nt_socket sockfd=%d\n", client_sockfd);
		}
	
	// }


	// test for write payload that bytes length > 256
	char data[512];
	memset(data, 0, sizeof(data));
	int unit_offset = strlen(large_msg);
	size_t data_len = strlen(large_msg) * 17;
	for (size_t i = 0; i < 17; i++)
	{
		memcpy(data + i * unit_offset, large_msg, unit_offset);
	}
	
	size_t sent_bytes_len;
	sent_bytes_len = write(client_sockfd, data, data_len);
	if (sent_bytes_len > 0) {
		printf("write large message [%d bytes] success \n", (int) sent_bytes_len);
		for (size_t i = 0; i < sent_bytes_len; i++)
		{
			printf("%c", data[i]);
		}
		printf("\n");
		
	}



	// test for write first in server
	// size_t write_msg_len;
	// write_msg_len = write(client_sockfd, test_msg, strlen(test_msg));
	// if (write_msg_len > 0) {
	// 	printf("write() success with send msg = '%s'\n", test_msg);
	// }


	char recv_msg[32] = {0};
	size_t read_msg_len;
	// read_msg_len = read(client_sockfd, recv_msg, sizeof(recv_msg));
	// if (read_msg_len > 0) {
	// 	printf("read() success with recv msg = '%s' \n", recv_msg);
	// }


	read(client_sockfd, recv_msg, sizeof(recv_msg));
	close(client_sockfd);

	close(sockfd);
    
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