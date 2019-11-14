/*
 * <p>Title: socket_test.cpp</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 14, 2019 
 * @version 1.0
 */

#include "gtest/gtest.h"

extern "C" {
#include "socket.h"
}

TEST(socket, print_socket) {
	ASSERT_EQ(print_socket(), 0);
}
