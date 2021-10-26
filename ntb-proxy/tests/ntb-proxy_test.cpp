#include <gtest/gtest.h>

extern "C" {
#include "ntb-proxy.h"
}

TEST(NtbProxyTest, print_hello) {
	int c = print_hello();
	EXPECT_EQ(c, 0);
}

TEST(NtbProxyTest, add) {
	int c = add(1,2);
	EXPECT_EQ(c, 3);
}


