#include <nyx/early_printk.h>
#include <nyx/early_serial.h>
#include <nyx/linkage.h>
#include <nyx/string.h>
#include <nyx/types.h>

static void __init early_internal_print_str(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) { early_serial_putc(str[i]); }
}

static char buffer[256] __initdata;

int __init early_vprintk(const char *fmt, va_list args) {
    int len;

    len = vsprintf(buffer, fmt, args);
    early_internal_print_str(buffer, len);

    return len;
}

int __init early_printk(const char *fmt, ...) {
    int len;

    va_list args;
    va_start(args, fmt);

    len = early_vprintk(fmt, args);

    va_end(args);
    return len;
}
