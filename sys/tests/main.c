#include <sys/printk.h>
#include <sys/stdarg.h>
#include <sys/string.h>
#include <sys/testing.h>

extern struct kernel_test __kernel_tests_start[];
extern struct kernel_test __kernel_tests_end[];

static void test_log(struct test_state *state, const char *s) {
    size_t room = TEST_LOG_SIZE - 1 - state->loglen;
    size_t len  = strlen(s);

    if (len > room) { len = room; }
    memcpy(state->log + state->loglen, s, len);
    state->loglen += len;
    state->log[state->loglen] = '\0';
}

void __test_fail(struct test_state *state, const char *file, int line, const char *fmt, ...) {
    char    msg[512];
    va_list args;

    sprintf(msg, "    | %s:%d: ", file, line);
    test_log(state, msg);

    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);

    test_log(state, msg);
    test_log(state, "\n");

    state->failed = true;
    state->nfailures++;
}

void __test_fail_mem(struct test_state *state,
                     const char        *file,
                     int                line,
                     const char        *aexpr,
                     const char        *bexpr,
                     const void        *a,
                     const void        *b,
                     size_t             len) {
    const unsigned char *ap = a, *bp = b;
    size_t               off = 0;

    while (off < len && ap[off] == bp[off]) { off++; }

    __test_fail(state,
                file,
                line,
                "expected " __TEST_EXPR " and " __TEST_EXPR " to match for %ld bytes, first difference at offset %ld: "
                "%#x vs %#x",
                aexpr,
                bexpr,
                (long) len,
                (long) off,
                ap[off],
                bp[off]);
}

int __test_strcmp(const char *a, const char *b) {
    if (!a || !b) { return a != b; }
    return strcmp(a, b);
}

void __do_kernel_tests() {
    struct test_state current_test_state;
    size_t            current_test, test_cnt, failed_cnt;

    current_test = 1;
    failed_cnt   = 0;
    test_cnt     = (size_t) ((char *) __kernel_tests_end - (char *) __kernel_tests_start) / sizeof(struct kernel_test);
    printk("Running %d tests:\n", test_cnt);

    for (struct kernel_test *t = __kernel_tests_start; t < __kernel_tests_end; ++t) {
        memset(&current_test_state, 0, sizeof(struct test_state));
        current_test_state.test_name = t->name;
        t->fn(&current_test_state);
        printk("[%d/%d]", current_test, test_cnt);
        if (current_test_state.failed) {
            printk("[FAILED] %s (%d failure%s)\n%s",
                   t->name,
                   current_test_state.nfailures,
                   current_test_state.nfailures == 1 ? "" : "s",
                   current_test_state.log);
            if (current_test_state.loglen == TEST_LOG_SIZE - 1) { printk("    | (log truncated)\n"); }
            failed_cnt++;
        } else {
            printk("[SUCCESS] %s\n", t->name);
        }
        current_test++;
    }

    printk("%d/%d tests passed, %d failed\n", test_cnt - failed_cnt, test_cnt, failed_cnt);
}
