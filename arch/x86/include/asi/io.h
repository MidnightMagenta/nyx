#ifndef _ASI_IO_H
#define _ASI_IO_H

#include <nyx/types.h>

#define outb(port, value)                                                                                              \
    __asm__ volatile("outb %b0, %w1" ::"a"(value), "Nd"(port)                                                          \
                     : "memory");

#define inb(port)                                                                                                      \
    ({                                                                                                                 \
        u8 _v;                                                                                                         \
        __asm__ volatile("inb %w1, %b0"                                                                                \
                         : "=a"(_v)                                                                                    \
                         : "Nd"(port)                                                                                  \
                         : "memory");                                                                                  \
        _v;                                                                                                            \
    })

static inline void outb_p(int port, u8 data) {
    outb(port, data);
    // HACK: outb 0x80 for IO delay.
    outb(0x80, 0);
}

static inline u8 inb_p(int port) {
    u8 data = inb(port);
    // HACK: outb 0x80 for IO delay
    outb(0x80, 0);
    return data;
}

#endif
