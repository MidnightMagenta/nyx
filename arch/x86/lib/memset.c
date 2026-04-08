#include <nyx/stddef.h>
#include <nyx/types.h>

// TODO: rep stosq would likely be better later on
void *memset(void *ptr, int v, size_t num) {
    unsigned char *p = ptr;

    while (((uintptr_t) p & (sizeof(uintptr_t) - 1)) && num) {
        *p++ = (unsigned char) v;
        num--;
    }

    uintptr_t word = (unsigned char) v;
    word |= word << 8;
    word |= word << 16;
    word |= word << 32;

    uintptr_t *wp = (uintptr_t *) p;
    while (num >= sizeof(uintptr_t)) {
        *wp++ = word;
        num -= sizeof(uintptr_t);
    }

    p = (unsigned char *) wp;
    while (num--) { *p++ = (unsigned char) v; }

    return ptr;
}
