#include <nyx/string.h>

size_t strnlen(const char *s, size_t count) {
    const char *sc;

    for (sc = s; count-- && *sc != '\0'; ++sc) { /* void */ }
    return sc - s;
}
