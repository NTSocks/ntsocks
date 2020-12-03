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

    int sockfd1, sockfd2;
    sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd1 > 0)
	{
		printf("sockfd1=%d \n", sockfd1);
		printf("socket() success\n");
	} else {
		printf("socket() failed.\n");
        return -1;
	}

    sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd1 > 0)
	{
		printf("sockfd2=%d \n", sockfd2);
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
	server_saddrlen = sizeof(server_saddr);

    retval = connect(sockfd1, (struct sockaddr *)&server_saddr, server_saddrlen);
    if (retval == -1) {
        printf("connect() failed.\n");
        return -1;
    } else {
        printf("connect() success\n");
    }

    retval = connect(sockfd2, (struct sockaddr *)&server_saddr, server_saddrlen);
    if (retval == -1) {
        printf("connect() failed.\n");
        return -1;
    } else {
        printf("connect() success\n");
    }

    char recv_data[512];
	memset(recv_data, 0, sizeof(recv_data));
	size_t recv_bytes_len;
	recv_bytes_len = read(sockfd1, recv_data, 512);
	if (recv_bytes_len > 0) {
		printf("read large message [%d bytes] success \n", (int)recv_bytes_len);
		for (size_t i = 0; i < recv_bytes_len; i++)
		{
			printf("%c", recv_data[i]);
		}
		printf("\n");
	}

    memset(recv_data, 0, sizeof(recv_data));
	recv_bytes_len = read(sockfd2, recv_data, 512);
	if (recv_bytes_len > 0) {
		printf("read large message [%d bytes] success \n", (int)recv_bytes_len);
		for (size_t i = 0; i < recv_bytes_len; i++)
		{
			printf("%c", recv_data[i]);
		}
		printf("\n");
	}

    close(sockfd1);
    close(sockfd2);

    printf("data transfer complete");

    getchar();

    return 0;
}
