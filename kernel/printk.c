#include <nyx/printk.h>
#include <stddef.h>

static int printk_ready = 0;

int vprintk(const char *fmt, va_list params) {
    // TODO: implement printk()
    (void) fmt;
    (void) params;
    return 0;
}

extern size_t early_vprintk(const char *fmt, va_list params);

int printk(const char *fmt, ...) {
    va_list params;
    int     written;

    va_start(params, fmt);

    if (printk_ready) {
        written = vprintk(fmt, params);
    } else {
        written = early_vprintk(fmt, params);
    }
    va_end(params);
    return (int) written;
}
