#include <sys/string.h>

const char *strchr(const char *s, int z) {
    for (;; s++) {
        if (*s == (char) z) { return s; }
        if (*s == '\0') { return NULL; }
    }
}

const char *strrchr(const char *s, int z) {
    const char *last = NULL;

    for (;; s++) {
        if (*s == (char) z) { last = s; }
        if (*s == '\0') { return last; }
    }
}
