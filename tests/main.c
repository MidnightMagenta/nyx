#include <nyx/printk.h>
#include <nyx/sprintk.h>
#include <nyx/testing.h>
#include <stdarg.h>
#include <string.h>

extern struct kernel_test __kernel_tests_start[];
extern struct kernel_test __kernel_tests_end[];

void __fail_test(struct test_state *state, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vsprintk(state->emsg, fmt, args);

    va_end(args);

    state->failed = 1;
}

void __do_kernel_tests() {
    struct test_state current_test_state;

    for (struct kernel_test *t = __kernel_tests_start; t < __kernel_tests_end; ++t) {
        memset(&current_test_state, 0, sizeof(struct test_state));
        current_test_state.test_name = t->name;
        t->fn(&current_test_state);
        if (current_test_state.failed) {
            printk("[FAILED] %s\n    | %s\n", t->name, current_test_state.emsg);
        } else {
            printk("[SUCCESS] %s\n", t->name);
        }
    }
}
