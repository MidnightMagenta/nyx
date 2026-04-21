#include <nyx/string.h>

int strcmp(const char *a, const char *b) {
    size_t i   = 0;
    size_t res = 0;
    while ((a[i] == b[i]) && (a[i] != '\0') && (b[i] != '\0')) { i++; }
    res = ((unsigned char) a[i] - (unsigned char) b[i]);
    return (int) res;
}
