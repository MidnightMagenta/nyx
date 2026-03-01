#include <string.h>

size_t strlen(const char *s) {
    size_t len = 0;
    if (s) {
        while (s[len] != '\0') { len++; }
    }
    return len;
}
