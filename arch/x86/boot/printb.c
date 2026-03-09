#define outb(port, value) __asm__ volatile("outb %b0, %w1" ::"a"(value), "Nd"(port) : "memory");
#define inb(port)                                                                                                      \
    ({                                                                                                                 \
        unsigned char _v;                                                                                              \
        __asm__ volatile("inb %w1, %b0" : "=a"(_v) : "Nd"(port) : "memory");                                           \
        _v;                                                                                                            \
    })

#define COM1_PORT     0x3F8
#define COM1_REG(reg) ((unsigned short) (COM1_PORT + reg))

#define COM_R_RX_BUFF                      0
#define COM_W_TX_BUFF                      0
#define COM_RW_INTERUPT_ENABLE_REG         1
#define COM_RW_LEAST_SIGNIFICANT_BYTE_BAUD 0
#define COM_RW_MOST_SIGNIFICANT_BYTE_BAUD  1
#define COM_R_INT_ID                       2
#define COM_W_FIFO_CONTROL_REG             2
#define COM_RW_LINE_CONTROL_REG            3
#define COM_RW_MODEM_CONTROL_REG           4
#define COM_R_LINE_STATUS_REG              5
#define COM_R_MODEM_STATUS_REG             6
#define COM_RW_SCRATCH                     7

static bool boot_serial_initialized = false;

int boot_serial_init() {
    if (boot_serial_initialized) { return 0; }

    outb(COM1_REG(COM_RW_INTERUPT_ENABLE_REG), 0x00);
    outb(COM1_REG(COM_RW_LINE_CONTROL_REG), 0x80);
    outb(COM1_REG(COM_RW_LEAST_SIGNIFICANT_BYTE_BAUD), 0x03);
    outb(COM1_REG(COM_RW_MOST_SIGNIFICANT_BYTE_BAUD), 0x00);
    outb(COM1_REG(COM_RW_LINE_CONTROL_REG), 0x03);
    outb(COM1_REG(COM_W_FIFO_CONTROL_REG), 0xC7);
    outb(COM1_REG(COM_RW_MODEM_CONTROL_REG), 0x1E);

    outb(COM1_REG(COM_W_TX_BUFF), 0xAE);
    if (inb(COM1_REG(COM_R_RX_BUFF)) != 0xAE) { return -1; }

    outb(COM1_REG(COM_RW_MODEM_CONTROL_REG), 0x0F);
    boot_serial_initialized = true;
    return 0;
}

static inline bool boot_serial_is_tx_empty() {
    return inb(COM1_REG(COM_R_LINE_STATUS_REG)) & 0x20;
}

void boot_serial_putc(char c) {
    if (!boot_serial_initialized) {
        if (boot_serial_init() < 0) { return; }
    }

    if (c == '\n') { boot_serial_putc('\r'); }
    while (!boot_serial_is_tx_empty());
    outb(COM1_REG(COM_W_TX_BUFF), (unsigned char) c);
}

void bputs(const char *s, unsigned long len) {
    for (unsigned long i = 0; i < len; ++i) { boot_serial_putc(s[i]); }
}

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)         __builtin_va_end(ap)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)

#define INT_MAX ((int) ((~0U) >> 1))

char buf_to_u64[128];
char buf_to_h64[128];
char buf_to_h32[128];
char buf_to_h16[128];
char buf_to_h8[128];
char buf_to_i64[128];

static const char *u64_to_str(unsigned long v) {
    unsigned long intest = v;
    unsigned char length = 0;
    while (intest / 10 > 0) {
        intest /= 10;
        length++;
    }

    unsigned char index = 0;
    do {
        unsigned char remainder = (unsigned char) (v % 10);
        v /= 10;
        buf_to_u64[length - index] = (char) (remainder) + '0';
        index++;
    } while (v / 10 > 0);
    unsigned char remainder    = (unsigned char) (v % 10);
    buf_to_u64[length - index] = (char) (remainder) + '0';
    buf_to_u64[length + 1]     = '\0';
    return buf_to_u64;
}

static const char *u64_to_hstr(unsigned long v) {
    static const char hexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) { buf_to_h64[15 - i] = hexDigits[(v >> (i * 4)) & 0xF]; }
    buf_to_h64[16] = '\0';
    return buf_to_h64;
}

static const char *u32_to_hstr(unsigned int v) {
    static const char hexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; ++i) { buf_to_h32[7 - i] = hexDigits[(v >> (i * 4)) & 0xF]; }
    buf_to_h32[8] = '\0';
    return buf_to_h32;
}

static const char *u8_to_hstr(unsigned char v) {
    static const char hexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 2; ++i) { buf_to_h8[1 - i] = hexDigits[(v >> (i * 4)) & 0xF]; }
    buf_to_h8[8] = '\0';
    return buf_to_h8;
}

static int bstrlen(const char *s) {
    int len = 0;
    while (*s) { len++; }
    return len;
}

static const char *s64_to_str(long v) {
    unsigned char negative = 0;
    if (v < 0) {
        negative = 1;
        v *= -1;
        buf_to_i64[0] = '-';
    }
    unsigned long intest = (unsigned long) (v);
    unsigned char length = 0;
    while (intest / 10 > 0) {
        intest /= 10;
        length++;
    }

    unsigned char index = 0;
    while (v / 10 > 0) {
        unsigned char remainder = (unsigned char) (v % 10);
        v /= 10;
        buf_to_i64[negative + length - index] = (char) (remainder) + '0';
        index++;
    }
    unsigned char remainder               = (unsigned char) (v % 10);
    buf_to_i64[negative + length - index] = (char) (remainder) + '0';
    buf_to_i64[negative + length + 1]     = '\0';
    return buf_to_i64;
}

int vprintb(const char *fmt, va_list params) {
    int written = 0;

    while (*fmt) {
        int maxrem = INT_MAX - written;

        if (fmt[0] != '%' || fmt[1] == '%') {
            if (fmt[0] == '%') { fmt++; }
            int amount = 1;
            while (fmt[amount] && fmt[amount] != '%') { amount++; }
            if (maxrem < amount) { return (int) 0; }
            bputs(fmt, amount);
            fmt += amount;
            written += amount;
            continue;
        }

        const char *fmt_begun_at = fmt++;

        if (*fmt == 'c') {
            fmt++;
            char c = (char) va_arg(params, int);
            if (maxrem < 1) { return (int) 0; }
            bputs(&c, 1);
            written++;
        } else if (*fmt == 's') {
            fmt++;
            const char *str    = va_arg(params, const char *);
            int         amount = bstrlen(str);
            if (maxrem < amount) { return (int) 0; }
            bputs(str, amount);
            written += amount;
        } else if (*fmt == 'd' || *fmt == 'i') {
            fmt++;
            int         num    = (int) va_arg(params, int);
            const char *str    = s64_to_str((long) num);
            int         amount = bstrlen(str);
            if (maxrem < amount) { return (int) 0; }
            bputs(str, amount);
            written += amount;
        } else if ((*fmt == 'l' && fmt[1] == 'd') || (*fmt == 'l' && fmt[1] == 'i')) {
            fmt += 2;
            long        num    = (long) va_arg(params, long);
            const char *str    = s64_to_str((long) num);
            int         amount = bstrlen(str);
            if (maxrem < amount) {
                // TODO: error
                return (int) 0;
            }
            bputs(str, amount);
            written += amount;
        } else if (*fmt == 'u') {
            fmt++;
            unsigned int num    = (unsigned int) va_arg(params, unsigned int);
            const char  *str    = u64_to_str((unsigned long) num);
            int          amount = bstrlen(str);
            if (maxrem < amount) { return (int) 0; }
            bputs(str, amount);
            written += amount;
        } else if ((*fmt == 'l' && fmt[1] == 'u')) {
            fmt += 2;
            unsigned long num    = (unsigned long) va_arg(params, unsigned long);
            const char   *str    = u64_to_str((unsigned long) num);
            int           amount = bstrlen(str);
            if (maxrem < amount) { return (int) 0; }
            bputs(str, amount);
            written += amount;
        } else if (*fmt == 'b') {
            fmt++;
            unsigned int num    = (unsigned int) va_arg(params, unsigned int);
            const char  *str    = u8_to_hstr(num);
            int          amount = bstrlen(str);
            if (maxrem < amount) { return (int) 0; }
            bputs(str, amount);
            written += amount;
        } else if (*fmt == 'x') {
            fmt++;
            unsigned int num    = (unsigned int) va_arg(params, unsigned int);
            const char  *str    = u32_to_hstr(num);
            int          amount = bstrlen(str);
            if (maxrem < amount) { return (int) 0; }
            bputs(str, amount);
            written += amount;
        } else if ((*fmt == 'l' && fmt[1] == 'x')) {
            fmt += 2;
            unsigned long num    = (unsigned long) va_arg(params, unsigned long);
            const char   *str    = u64_to_hstr(num);
            int           amount = bstrlen(str);
            if (maxrem < amount) { return (int) 0; }
            bputs(str, amount);
            written += amount;
        } else {
            fmt        = fmt_begun_at;
            int amount = bstrlen(fmt);
            if (maxrem < amount) { return (int) 0; }
            bputs(fmt, amount);
            written += amount;
            fmt += amount;
        }
    }

    return written;
}

int printb(const char *fmt, ...) {
    va_list params;
    va_start(params, fmt);

    int written = vprintb(fmt, params);

    va_end(params);
    return written;
}
