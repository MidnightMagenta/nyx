#include <sys/testing.h>

KERNEL_TEST(sanity_test) {
    ASSERT_EQ(1 + 1, 2);
    ASSERT_TRUE(true);
    ASSERT_FALSE(false);
}
