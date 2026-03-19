#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

// int    memcmp(const void *a, const void *b, size_t num);
void *memcpy(void *restrict dest, const void *restrict src, size_t len);
void *memset(void *ptr, int v, size_t num);
void *memmove(void *dest, const void *src, size_t len);
// int    strcmp(const char *a, const char *b);
// char  *strcpy(char *restrict dst, char *restrict src);
size_t strlen(const char *s);


#endif
