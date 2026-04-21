#include <nyx/printk.h>
#include <nyx/stdarg.h>
#include <nyx/string.h>
#include <nyx/testing.h>

extern struct kernel_test __kernel_tests_start[];
extern struct kernel_test __kernel_tests_end[];

void __fail_test(struct test_state *state, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vsprintf(state->emsg, fmt, args);

    va_end(args);

    state->failed = 1;
}

void __do_kernel_tests() {
    struct test_state current_test_state;
    size_t            current_test, test_cnt;

    current_test = 1;
    test_cnt     = (size_t) ((char *) __kernel_tests_end - (char *) __kernel_tests_start) / sizeof(struct kernel_test);
    printk("Running %d tests:\n", test_cnt);

    for (struct kernel_test *t = __kernel_tests_start; t < __kernel_tests_end; ++t) {
        memset(&current_test_state, 0, sizeof(struct test_state));
        current_test_state.test_name = t->name;
        t->fn(&current_test_state);
        printk("[%d/%d]", current_test, test_cnt);
        if (current_test_state.failed) {
            printk("[FAILED] %s\n    | %s\n", t->name, current_test_state.emsg);
        } else {
            printk("[SUCCESS] %s\n", t->name);
        }
        current_test++;
    }
}
