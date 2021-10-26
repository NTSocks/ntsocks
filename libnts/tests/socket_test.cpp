#include <gtest/gtest.h>

#include "socket.h"
#include "ntm_shm.h"


TEST(socket, print_socket) {
	ASSERT_EQ(print_socket(), 0);
}
