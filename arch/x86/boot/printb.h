#ifndef _BOOT_PRINTB_H
#define _BOOT_PRINTB_H

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)         __builtin_va_end(ap)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)

#define pr_fmt(msg) "boot: " msg "\n"

int  boot_serial_init();
void boot_serial_putc(char c);
void boot_serial_putc(char c);
int  vprintb(const char *fmt, va_list params);
int  printb(const char *fmt, ...);

#define pr_error(f, ...) printb("Error (%s:%d): " f, __FILE__, __LINE__, ##__VA_ARGS__)
#define pr_warn(f, ...)  printb("Warning: " f, ##__VA_ARGS__)
#define pr_info(f, ...)  printb("Info: " f, ##__VA_ARGS__)

#ifdef CONFIG_DEBUG
#define pr_dbg(f, ...) printb(f, ##__VA_ARGS__)
#else
#define pr_dbg(f, ...)
#endif

#endif
