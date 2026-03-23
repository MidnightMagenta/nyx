#include <nyx/sprintk.h>

// TODO: implement vsprintk
int vsprintk(char *dst, const char *fmt, va_list args) {
    long int arg;
    int      len, sign, i;
    char    *p, *orig = dst, tmpstr[19], pad = ' ';

    if (dst == (void *) 0 || fmt == (void *) 0) return 0;

    arg = 0;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '%') goto put;

            len = 0;
            if (*fmt == '0') pad = '0';

            while (*fmt >= '0' && *fmt <= '9') {
                len *= 10;
                len += *fmt - '0';
                fmt++;
            }

            if (*fmt == 'l') fmt++;

            if (*fmt == 'c') {
                arg    = va_arg(args, int);
                *dst++ = (char) arg;
                fmt++;
                continue;
            } else if (*fmt == 'd') {
                arg  = va_arg(args, int);
                sign = 0;
                if ((int) arg < 0) {
                    arg *= -1;
                    sign++;
                }

                if (arg > 99999999999999999L) { arg = 99999999999999999L; }

                i         = 18;
                tmpstr[i] = 0;
                do {
                    tmpstr[--i] = '0' + (arg % 10);
                    arg /= 10;
                } while (arg != 0 && i > 0);

                if (sign) { tmpstr[--i] = '-'; }

                if (len > 0 && len < 18) {
                    while (i > 18 - len) { tmpstr[--i] = pad; }
                }

                p = &tmpstr[i];
                goto copystring;
            } else if (*fmt == 'x') {
                arg       = va_arg(args, long int);
                i         = 16;
                tmpstr[i] = 0;
                do {
                    char n      = arg & 0xf;
                    tmpstr[--i] = n + (n > 9 ? 0x37 : 0x30);
                    arg >>= 4;
                } while (arg != 0 && i > 0);

                if (len > 0 && len <= 16) {
                    while (i > 16 - len) { tmpstr[--i] = '0'; }
                }

                p = &tmpstr[i];
                goto copystring;
            } else if (*fmt == 's') {
                p = va_arg(args, char *);

            copystring:
                if (p == (void *) 0) { p = "(null)"; }
                while (*p) { *dst++ = *p++; }
            }
        } else {
        put:
            *dst++ = *fmt;
        }

        fmt++;
    }

    *dst = 0;
    return dst - orig;
}

int sprintk(char *dst, const char *fmt, ...) {
    va_list args;
    int     written;

    va_start(args, fmt);

    written = vsprintk(dst, fmt, args);

    va_end(args);
    return written;
}
