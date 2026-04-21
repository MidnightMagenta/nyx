#include <asi/system.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <nyx/printk.h>
#include <nyx/stdarg.h>
#include <nyx/stddef.h>

extern size_t early_vprintk(const char *fmt, va_list params);

void __init early_panic(const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);

    vprintk(fmt, params);
    printk("\n");

    va_end(params);

    hcf();
}
