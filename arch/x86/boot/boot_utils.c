#include "boot_utils.h"

void *memsetb(void *p, int v, size_t c) {
    unsigned char *strPtr = (unsigned char *) p;
    while (c--) { *strPtr++ = (unsigned char) v; }
    return p;
}

void *memmoveb(void *dest, const void *src, size_t len) {
    char       *d = dest;
    const char *s = src;
    if (d < s)
        while (len--) *d++ = *s++;
    else {
        char *lasts = s + (len - 1);
        char *lastd = d + (len - 1);
        while (len--) *lastd-- = *lasts--;
    }
    return dest;
}
