#include <nyx/printk.h>
#include <nyx/stddef.h>

int vprintk(const char *fmt, va_list args) {
    // TODO: implement printk()
    (void) fmt;
    (void) args;
    return 0;
}

extern int early_vprintk(const char *fmt, va_list args);

static int (*vprintk_impl)(const char *, va_list) = early_vprintk;

int printk(const char *fmt, ...) {
    va_list args;
    int     written;

    va_start(args, fmt);

    written = vprintk_impl(fmt, args);

    va_end(args);
    return (int) written;
}
