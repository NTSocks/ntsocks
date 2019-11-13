#include "gtest/gtest.h"

extern "C" {
#include "func.h"
}

TEST(func, func) {
    ASSERT_EQ(1, func(1,2));
}
