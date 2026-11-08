#ifndef _BOOT_PRINTB_H
#define _BOOT_PRINTB_H

#include <nyx/stdarg.h>

int  boot_serial_init();
void boot_serial_putc(char c);
void boot_serial_putc(char c);
int  vprintb(const char *fmt, va_list params);
int  printb(const char *fmt, ...);

#endif
