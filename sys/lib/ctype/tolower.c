#include <nyx/ctype.h>

int tolower(int ch) {
    if (isupper(ch)) { return ch | 32; }
    return ch;
}
