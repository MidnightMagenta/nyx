#include <nyx/string.h>

void *memset(void *ptr, int v, size_t num) {
    unsigned char *strPtr = (unsigned char *) ptr;
    while (num--) { *strPtr++ = (unsigned char) v; }
    return ptr;
}
