/*
 * <p>Title: hello-ntsock.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */

#include "socket.h"

int main(int argc, char * argv[]) {
	printf("Hello libnts app!\n");

//	print_socket();
	test_ntm_shm();

	printf("Bye, libnts app.\n");

	return 0;
}
