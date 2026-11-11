#include <asi/system.h>
#include <sys/linkage.h>
#include <sys/panic.h>
#include <sys/printk.h>
#include <sys/stdarg.h>
#include <sys/stddef.h>

extern size_t early_vprintk(const char *fmt, va_list params);

void __init early_panic(const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);

    early_vprintk(fmt, params);
    printk("\n");

    va_end(params);

    hcf();
}
