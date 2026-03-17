#include <asi/system.h>
#include <nyx/early_printk.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <stdarg.h>
#include <stddef.h>

extern size_t early_vprintk(const char *fmt, va_list params);

void __init early_panic(const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);

    early_vprintk(fmt, params);
    early_printk("\n");

    va_end(params);

    hcf();
}
