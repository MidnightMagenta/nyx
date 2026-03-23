#ifndef _NYX_SPRINTK_H
#define _NYX_SPRINTK_H

#include <stdarg.h>

int vsprintk(char *dst, const char *fmt, va_list args);
int sprintk(char *dst, const char *fmt, ...);

#endif
