#ifndef _NYX_PRINTK_H
#define _NYX_PRINTK_H

#include <sys/stdarg.h>

int vprintk(const char *fmt, va_list args);
int printk(const char *fmt, ...);

#endif
