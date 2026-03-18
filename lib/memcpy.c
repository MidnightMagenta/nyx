#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t len) {
    unsigned char       *dstPtr = (unsigned char *) dest;
    const unsigned char *srcPtr = (const unsigned char *) src;

    while (len--) { *dstPtr++ = *srcPtr++; }
    return dest;
}
