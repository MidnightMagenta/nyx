#ifndef _NYX_PRINTK_H
#define _NYX_PRINTK_H

#include <stdarg.h>

int vprintk(const char *fmt, va_list args);
int printk(const char *fmt, ...);

#define pr_error(f, ...) printk("error [%s:%d]: " f, __FILE__, __LINE__, ##__VA_ARGS__)
#define pr_warn(f, ...)  printk("warning: " f, ##__VA_ARGS__)
#define pr_info(f, ...)  printk("info: " f, ##__VA_ARGS__)
#ifdef CONFIG_DEBUG
#define pr_dbg(f, ...) printk("debug [%s:%d]: " f, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define pr_dbg(f, ...)
#endif

#endif
