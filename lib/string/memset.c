#include <nyx/compiler.h>
#include <nyx/string.h>

void *__memset_generic(void *ptr, int v, size_t num) {
    unsigned char *strPtr = (unsigned char *) ptr;
    while (num--) { *strPtr++ = (unsigned char) v; }
    return ptr;
}

void *memset(void *ptr, int v, size_t num) __weak_alias("__memset_generic");
