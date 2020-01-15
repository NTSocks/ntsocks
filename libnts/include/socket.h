/*
 * <p>Title: socket.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


extern int print_socket();

extern int test_ntm_shm();

extern void test_nts_shm();

#ifdef __cplusplus
};
#endif

#endif /* SOCKET_H_ */
