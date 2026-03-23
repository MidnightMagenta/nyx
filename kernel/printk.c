#include <nyx/printk.h>
#include <stddef.h>

static int printk_ready = 0;

int vprintk(const char *fmt, va_list args) {
    // TODO: implement printk()
    (void) fmt;
    (void) args;
    return 0;
}

extern size_t early_vprintk(const char *fmt, va_list args);

int printk(const char *fmt, ...) {
    va_list args;
    int     written;

    va_start(args, fmt);

    if (printk_ready) {
        written = vprintk(fmt, args);
    } else {
        written = early_vprintk(fmt, args);
    }
    va_end(args);
    return (int) written;
}
