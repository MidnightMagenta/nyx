#include <nyx/early_printk.h>
#include <nyx/early_serial.h>
#include <nyx/linkage.h>
#include <nyx/types.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

char __initdata buf_to_u64[128];
char __initdata buf_to_h64[128];
char __initdata buf_to_h32[128];
char __initdata buf_to_h16[128];
char __initdata buf_to_h8[128];
char __initdata buf_to_i64[128];

static const char *__init u64_to_str(u64 v) {
    u64 size_test = v;
    u8  length    = 0;
    while (size_test / 10 > 0) {
        size_test /= 10;
        length++;
    }

    u8 index = 0;
    do {
        u8 remainder = (u8) (v % 10);
        v /= 10;
        buf_to_u64[length - index] = (char) (remainder) + '0';
        index++;
    } while (v / 10 > 0);
    u8 remainder               = (u8) (v % 10);
    buf_to_u64[length - index] = (char) (remainder) + '0';
    buf_to_u64[length + 1]     = '\0';
    return buf_to_u64;
}

static const char *__init u64_to_hstr(u64 v) {
    static const char hexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) { buf_to_h64[15 - i] = hexDigits[(v >> (i * 4)) & 0xF]; }
    buf_to_h64[16] = '\0';
    return buf_to_h64;
}

static const char *__init u32_to_hstr(u32 v) {
    static const char hexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; ++i) { buf_to_h32[7 - i] = hexDigits[(v >> (i * 4)) & 0xF]; }
    buf_to_h32[8] = '\0';
    return buf_to_h32;
}

static const char *__init u8_to_hstr(u8 v) {
    static const char hexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 2; ++i) { buf_to_h8[1 - i] = hexDigits[(v >> (i * 4)) & 0xF]; }
    buf_to_h8[8] = '\0';
    return buf_to_h8;
}

static const char *__init s64_to_str(s64 v) {
    u8 negative = 0;
    if (v < 0) {
        negative = 1;
        v *= -1;
        buf_to_i64[0] = '-';
    }
    u64 size_test = (u64) (v);
    u8  length    = 0;
    while (size_test / 10 > 0) {
        size_test /= 10;
        length++;
    }

    u8 index = 0;
    while (v / 10 > 0) {
        u8 remainder = (u8) (v % 10);
        v /= 10;
        buf_to_i64[negative + length - index] = (char) (remainder) + '0';
        index++;
    }
    u8 remainder                          = (u8) (v % 10);
    buf_to_i64[negative + length - index] = (char) (remainder) + '0';
    buf_to_i64[negative + length + 1]     = '\0';
    return buf_to_i64;
}

static void __init early_internal_print_str(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) { early_serial_putc(str[i]); }
}

size_t __init early_vprintk(const char *fmt, va_list params) {
    size_t written = 0;
    while (*fmt != '\0') {
        size_t maxrem = SIZE_MAX - written;

        if (fmt[0] != '%' || fmt[1] == '%') {
            if (fmt[0] == '%') { fmt++; }
            size_t amount = 1;
            while (fmt[amount] && fmt[amount] != '%') { amount++; }
            if (maxrem < amount) {
                // TODO: error stuff
                return (size_t) 0;
            }
            early_internal_print_str(fmt, amount);
            fmt += amount;
            written += amount;
            continue;
        }

        const char *fmt_begun_at = fmt++;

        if (*fmt == 'c') {
            fmt++;
            char c = (char) va_arg(params, int);
            if (maxrem < 1) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(&c, 1);
            written++;
        } else if (*fmt == 's') {
            fmt++;
            const char *str    = va_arg(params, const char *);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else if (*fmt == 'd' || *fmt == 'i') {
            fmt++;
            s32         num    = (s32) va_arg(params, s32);
            const char *str    = s64_to_str((s64) num);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else if ((*fmt == 'l' && fmt[1] == 'd') || (*fmt == 'l' && fmt[1] == 'i')) {
            fmt += 2;
            s64         num    = (s64) va_arg(params, s64);
            const char *str    = s64_to_str((s64) num);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else if (*fmt == 'u') {
            fmt++;
            u32         num    = (u32) va_arg(params, u32);
            const char *str    = u64_to_str((u64) num);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else if ((*fmt == 'l' && fmt[1] == 'u')) {
            fmt += 2;
            u64         num    = (u64) va_arg(params, u64);
            const char *str    = u64_to_str((u64) num);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else if (*fmt == 'b') {
            fmt++;
            u32         num    = (u32) va_arg(params, u32);
            const char *str    = u8_to_hstr(num);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else if (*fmt == 'x') {
            fmt++;
            u32         num    = (u32) va_arg(params, u32);
            const char *str    = u32_to_hstr(num);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else if ((*fmt == 'l' && fmt[1] == 'x')) {
            fmt += 2;
            u64         num    = (u64) va_arg(params, u64);
            const char *str    = u64_to_hstr(num);
            size_t      amount = strlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (size_t) 0;
            }
            early_internal_print_str(str, amount);
            written += amount;
        } else {
            fmt           = fmt_begun_at;
            size_t amount = strlen(fmt);
            if (maxrem < amount) {
                // implement errno
                return (size_t) 0;
            }
            early_internal_print_str(fmt, amount);
            written += amount;
            fmt += amount;
        }
    }

    return written;
}

int __init early_printk(const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);

    size_t written = early_vprintk(fmt, params);

    va_end(params);
    return (int) written;
}
