#ifdef __TESTING__

#ifndef _NYX_TESTING_H
#define _NYX_TESTING_H

struct test_state {
    const char *test_name;
    bool        failed;
    char        emsg[256];
};

struct kernel_test {
    void (*fn)(struct test_state *);
    const char name[64];
};

#define DEFINE_KERNEL_TEST(name, fn)                                                                                   \
    static const struct kernel_test __test_##name __attribute__((section(".kernel_tests"), used)) = {fn, #name};

#define KERNEL_TEST(name)                                                                                              \
    void __test_fn_##name(struct test_state *state);                                                                   \
    DEFINE_KERNEL_TEST(name, __test_fn_##name);                                                                        \
    void __test_fn_##name(struct test_state *state)

void __fail_test(struct test_state *state, const char *fmt, ...);

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

#endif
#endif
