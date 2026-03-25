#ifdef __TESTING__

#ifndef _NYX_TESTING_H
#define _NYX_TESTING_H

#include <nyx/printk.h>

struct test_state {
    const char *test_name;
    bool        failed;
    char        emsg[256];
};

struct kernel_test {
    void (*fn)(struct test_state *);
    const char name[64];
} __attribute__((packed, aligned(8)));

#define DEFINE_KERNEL_TEST(name, fn)                                                                                   \
    static const struct kernel_test __test_##name                                                                      \
            __attribute__((aligned(1), section(".kernel_tests"), used)) = {fn, #name};

#define KERNEL_TEST(name)                                                                                              \
    void __test_fn_##name(struct test_state *state);                                                                   \
    DEFINE_KERNEL_TEST(name, __test_fn_##name);                                                                        \
    void __test_fn_##name(struct test_state *state)

void __fail_test(struct test_state *state, const char *fmt, ...);

#define TEST_PRINT(fmt, ...)                                                                                           \
    do { printk("[TEST: %s] " fmt "\n", state->test_name, ##__VA_ARGS__); } while (0);

#define EXPECT_TRUE(c)                                                                                                 \
    if (!(c)) { __fail_test(state, "%s:%d: '%s' was false. Expected true.", __FILE__, __LINE__, #c); }

#define EXPECT_FALSE(c)                                                                                                \
    if (!!(c)) { __fail_test(state, "%s:%d: '%s' was true. Expected false.", __FILE__, __LINE__, #c); }

#define EXPECT_EQ(a, b)                                                                                                \
    if ((a) != (b)) {                                                                                                  \
        __fail_test(state,                                                                                             \
                    "%s:%d: '%s' was %ld. Expected %ld.",                                                              \
                    __FILE__,                                                                                          \
                    __LINE__,                                                                                          \
                    #a,                                                                                                \
                    (unsigned long) (a),                                                                               \
                    (unsigned long) (b));                                                                              \
        return;                                                                                                        \
    }

#define EXPECT_NEQ(a, b)                                                                                               \
    if ((a) == (b)) {                                                                                                  \
        __fail_test(state,                                                                                             \
                    "%s:%d: '%s' was %ld. Expected not %ld.",                                                          \
                    __FILE__,                                                                                          \
                    __LINE__,                                                                                          \
                    #a,                                                                                                \
                    (unsigned long) (a),                                                                               \
                    (unsigned long) (b));                                                                              \
        return;                                                                                                        \
    }

#endif
#endif
