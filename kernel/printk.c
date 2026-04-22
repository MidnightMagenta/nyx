#include <nyx/printk.h>

extern int early_vprintk(const char *fmt, va_list args);
static int (*vprintk_impl)(const char *, va_list) = early_vprintk;

int vprintk(const char *fmt, va_list args) {
    // TODO: implement vprintk()
    (void) fmt;
    (void) args;
    return 0;
}

int printk(const char *fmt, ...) {
    va_list args;
    int     written;

    va_start(args, fmt);

    written = vprintk_impl(fmt, args);

    va_end(args);
    return (int) written;
}
