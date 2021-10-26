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

#define SERVER_IP "10.10.88.210"
#define PORT 80

#define test_msg "Hello, NTSocket Server World!"
#define large_msg "Hello,Large Msg!"

int main(int argc, char *argv[])
{

	if (argc < 3)
	{
		printf("Usage: %s <Server IP> <Port> \n", argv[0]);
		return 0;
	}
	int port = atoi(argv[2]);
	char *server_ip = argv[1];

	int listen_sockfd;
	listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sockfd > 0)
	{
		printf("listen_sockfd=%d \n", listen_sockfd);
		printf("socket() success\n");
	}
	else
	{
		printf("socket() failed.\n");
		return -1;
	}

	int retval;
	socklen_t saddrlen;
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr(server_ip);
	saddrlen = sizeof(saddr);
	retval = bind(listen_sockfd, (struct sockaddr *)&saddr, saddrlen);
	if (retval == -1)
	{
		printf("bind() failed.\n");
		return -1;
	}
	else
	{
		printf("bind() success\n");
	}

	retval = listen(listen_sockfd, 8);
	if (retval == -1)
	{
		printf("listen() failed\n");
		return -1;
	}
	else
	{
		printf("listen() success\n");
	}

	int sockfd1, sockfd2;
	socklen_t client_saddrlen;
	struct sockaddr_in client_saddr;
	sockfd1 = accept(listen_sockfd,
					 (struct sockaddr *)&client_saddr, &client_saddrlen);
	if (sockfd1 == -1)
	{
		printf("accept() failed\n");
		return -1;
	}
	else
	{
		printf("accept() success with client nt_socket sockfd=%d\n", sockfd1);
	}

	sockfd2 = accept(listen_sockfd,
					 (struct sockaddr *)&client_saddr, &client_saddrlen);
	if (sockfd1 == -1)
	{
		printf("accept() failed\n");
		return -1;
	}
	else
	{
		printf("accept() success with client nt_socket sockfd=%d\n", sockfd2);
	}

	// ready to send payload into clients
	char data[512];
	memset(data, 0, sizeof(data));
	int unit_offset = strlen(large_msg);
	size_t data_len = strlen(large_msg) * 17;
	for (size_t i = 0; i < 17; i++)
	{
		memcpy(data + i * unit_offset, large_msg, unit_offset);
	}

	size_t sent_bytes_len;
	sent_bytes_len = write(sockfd1, data, data_len);
	if (sent_bytes_len > 0)
	{
		printf("write large message [%d bytes] success \n", (int)sent_bytes_len);
		for (size_t i = 0; i < sent_bytes_len; i++)
		{
			printf("%c", data[i]);
		}
		printf("\n");
	}

	sent_bytes_len = write(sockfd2, data, data_len);
	if (sent_bytes_len > 0)
	{
		printf("write large message [%d bytes] success \n", (int)sent_bytes_len);
		for (size_t i = 0; i < sent_bytes_len; i++)
		{
			printf("%c", data[i]);
		}
		printf("\n");
	}

	char recv_msg[32] = {0};
	size_t read_msg_len;

	read(sockfd1, recv_msg, sizeof(recv_msg));
	close(sockfd1);

	read(sockfd2, recv_msg, sizeof(recv_msg));
	close(sockfd2);

	close(listen_sockfd);

	printf("data transfer complete");

	getchar();

	return 0;
}