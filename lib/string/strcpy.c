#include <nyx/string.h>

char *strcpy(char *restrict dest, const char *restrict src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t len) {
    size_t size = strnlen(src, len);
    if (size != len) { memset(dest + size, '\0', len - size); }
    return memcpy(dest, src, size);
}
