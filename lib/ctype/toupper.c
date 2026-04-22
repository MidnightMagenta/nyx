#include <nyx/ctype.h>

int toupper(int ch) {
    if (islower(ch)) return ch & 0x5f;
    return ch;
}
