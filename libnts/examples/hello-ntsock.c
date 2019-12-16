/*
 * <p>Title: hello-ntsock.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */

//#include "socket.h"
//#include "nts_api.h"

#include <sys/socket.h>
#include <stdio.h>
//#include <unistd.h>


int main(int argc, char * argv[]) {
	printf("Hello libnts app!\n");

//	int sockfd;
	socket(AF_INET, SOCK_STREAM, 0);

//	close(sockfd);

//	print_socket();
//	test_ntm_shm();
//	nts_init(NTS_CONFIG_FILE);
//
//	getchar();
//
//	nts_destroy();

	printf("Bye, libnts app.\n");

	return 0;
}
