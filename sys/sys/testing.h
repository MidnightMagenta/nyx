#ifdef __TESTING__

#ifndef _NYX_TESTING_H
#define _NYX_TESTING_H

#include <sys/printk.h>
#include <sys/stddef.h>
#include <sys/string.h>
#include <sys/types.h>

#define TEST_LOG_SIZE 1024

struct test_state {
    const char *test_name;
    bool        failed;
    int         nfailures;
    size_t      loglen;
    char        log[TEST_LOG_SIZE];
};

struct kernel_test {
    void       (*fn)(struct test_state *);
    const char name[64];
} __attribute__((packed, aligned(8)));

#define DEFINE_KERNEL_TEST(name, fn)                                                                                   \
    static const struct kernel_test __test_##name                                                                      \
            __attribute__((aligned(1), section(".kernel_tests"), used)) = {fn, #name};

#define KERNEL_TEST(name)                                                                                              \
    void __test_fn_##name(struct test_state *state);                                                                   \
    DEFINE_KERNEL_TEST(name, __test_fn_##name);                                                                        \
    void __test_fn_##name(struct test_state *state)

void __test_fail(struct test_state *state, const char *file, int line, const char *fmt, ...);
void __test_fail_mem(struct test_state *state,
                     const char        *file,
                     int                line,
                     const char        *aexpr,
                     const char        *bexpr,
                     const void        *a,
                     const void        *b,
                     size_t             len);
int  __test_strcmp(const char *a, const char *b);

#define TEST_PRINT(fmt, ...)                                                                                           \
    do { printk("[TEST: %s] " fmt "\n", state->test_name, ##__VA_ARGS__); } while (0)

#define __TEST_EXPR "'%.96s'"

#define __TEST_ON_FAIL(fatal) __TEST_ON_FAIL_##fatal
#define __TEST_ON_FAIL_0      /* keep going */
#define __TEST_ON_FAIL_1      return

#define __TEST_BOOL(fatal, c, want)                                                                                    \
    do {                                                                                                               \
        if (!!(c) != (want)) {                                                                                         \
            __test_fail(state,                                                                                         \
                        __FILE__,                                                                                      \
                        __LINE__,                                                                                      \
                        __TEST_EXPR " was %s, expected %s",                                                            \
                        #c,                                                                                            \
                        (want) ? "false" : "true",                                                                     \
                        (want) ? "true" : "false");                                                                    \
            __TEST_ON_FAIL(fatal);                                                                                     \
        }                                                                                                              \
    } while (0)

#define __TEST_CMP(fatal, a, op, b)                                                                                    \
    do {                                                                                                               \
        __auto_type __ta = (a);                                                                                        \
        __auto_type __tb = (b);                                                                                        \
        if (!(__ta op __tb)) {                                                                                         \
            __test_fail(state,                                                                                         \
                        __FILE__,                                                                                      \
                        __LINE__,                                                                                      \
                        "expected " __TEST_EXPR " " #op " " __TEST_EXPR ", got %ld (%#lx) vs %ld (%#lx)",              \
                        #a,                                                                                            \
                        #b,                                                                                            \
                        (long) __ta,                                                                                   \
                        (unsigned long) __ta,                                                                          \
                        (long) __tb,                                                                                   \
                        (unsigned long) __tb);                                                                         \
            __TEST_ON_FAIL(fatal);                                                                                     \
        }                                                                                                              \
    } while (0)

#define __TEST_STR(fatal, a, b, want_eq)                                                                               \
    do {                                                                                                               \
        const char *__ta = (a);                                                                                        \
        const char *__tb = (b);                                                                                        \
        if ((__test_strcmp(__ta, __tb) == 0) != (want_eq)) {                                                           \
            __test_fail(state,                                                                                         \
                        __FILE__,                                                                                      \
                        __LINE__,                                                                                      \
                        "expected " __TEST_EXPR " %s " __TEST_EXPR ", got \"%.64s\" vs \"%.64s\"",                     \
                        #a,                                                                                            \
                        (want_eq) ? "==" : "!=",                                                                       \
                        #b,                                                                                            \
                        __ta,                                                                                          \
                        __tb);                                                                                         \
            __TEST_ON_FAIL(fatal);                                                                                     \
        }                                                                                                              \
    } while (0)

#define __TEST_MEM(fatal, a, b, len)                                                                                   \
    do {                                                                                                               \
        const void *__ta = (a);                                                                                        \
        const void *__tb = (b);                                                                                        \
        size_t      __tl = (len);                                                                                      \
        if (memcmp(__ta, __tb, __tl)) {                                                                                \
            __test_fail_mem(state, __FILE__, __LINE__, #a, #b, __ta, __tb, __tl);                                      \
            __TEST_ON_FAIL(fatal);                                                                                     \
        }                                                                                                              \
    } while (0)

#define __TEST_OK(fatal, x)                                                                                            \
    do {                                                                                                               \
        long __tx = (long) (x);                                                                                        \
        if (__tx != 0) {                                                                                               \
            __test_fail(state, __FILE__, __LINE__, __TEST_EXPR " returned %ld, expected 0", #x, __tx);                 \
            __TEST_ON_FAIL(fatal);                                                                                     \
        }                                                                                                              \
    } while (0)

#define ASSERT_TRUE(c)  __TEST_BOOL(1, c, true)
#define ASSERT_FALSE(c) __TEST_BOOL(1, c, false)
#define EXPECT_TRUE(c)  __TEST_BOOL(0, c, true)
#define EXPECT_FALSE(c) __TEST_BOOL(0, c, false)

#define ASSERT_EQ(a, b) __TEST_CMP(1, a, ==, b)
#define ASSERT_NE(a, b) __TEST_CMP(1, a, !=, b)
#define ASSERT_LT(a, b) __TEST_CMP(1, a, <, b)
#define ASSERT_LE(a, b) __TEST_CMP(1, a, <=, b)
#define ASSERT_GT(a, b) __TEST_CMP(1, a, >, b)
#define ASSERT_GE(a, b) __TEST_CMP(1, a, >=, b)
#define EXPECT_EQ(a, b) __TEST_CMP(0, a, ==, b)
#define EXPECT_NE(a, b) __TEST_CMP(0, a, !=, b)
#define EXPECT_LT(a, b) __TEST_CMP(0, a, <, b)
#define EXPECT_LE(a, b) __TEST_CMP(0, a, <=, b)
#define EXPECT_GT(a, b) __TEST_CMP(0, a, >, b)
#define EXPECT_GE(a, b) __TEST_CMP(0, a, >=, b)

#define ASSERT_NULL(p)     ASSERT_EQ((const void *) (p), NULL)
#define ASSERT_NOT_NULL(p) ASSERT_NE((const void *) (p), NULL)
#define EXPECT_NULL(p)     EXPECT_EQ((const void *) (p), NULL)
#define EXPECT_NOT_NULL(p) EXPECT_NE((const void *) (p), NULL)

#define ASSERT_OK(x) __TEST_OK(1, x)
#define EXPECT_OK(x) __TEST_OK(0, x)

#define ASSERT_STREQ(a, b) __TEST_STR(1, a, b, true)
#define ASSERT_STRNE(a, b) __TEST_STR(1, a, b, false)
#define EXPECT_STREQ(a, b) __TEST_STR(0, a, b, true)
#define EXPECT_STRNE(a, b) __TEST_STR(0, a, b, false)

#define ASSERT_MEMEQ(a, b, len) __TEST_MEM(1, a, b, len)
#define EXPECT_MEMEQ(a, b, len) __TEST_MEM(0, a, b, len)

#define FAIL(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        __test_fail(state, __FILE__, __LINE__, fmt, ##__VA_ARGS__);                                                    \
        return;                                                                                                        \
    } while (0)
#define ADD_FAILURE(fmt, ...)                                                                                          \
    do { __test_fail(state, __FILE__, __LINE__, fmt, ##__VA_ARGS__); } while (0)

#define TEST_HAS_FAILED() (state->failed)

#define ASSERT_NO_FAILURE(call)                                                                                        \
    do {                                                                                                               \
        call;                                                                                                          \
        if (TEST_HAS_FAILED()) { return; }                                                                             \
    } while (0)

#endif
#endif
