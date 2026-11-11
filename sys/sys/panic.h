#ifndef _NYX_PANIC_H
#define _NYX_PANIC_H

#include <sys/compiler.h>

void __noreturn early_panic(const char *fmt, ...);

#define panic(fmt, ...) early_panic(fmt, ##__VA_ARGS__)

#endif
