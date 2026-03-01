#ifndef _ASI_IO_H
#define _ASI_IO_H

#define outb(port, value) __asm__ volatile("outb %b0, %w1" ::"a"(value), "Nd"(port) : "memory");

#define inb(port)                                                                                                      \
    ({                                                                                                                 \
        u8 _v;                                                                                                         \
        __asm__ volatile("inb %w1, %b0" : "=a"(_v) : "Nd"(port) : "memory");                                           \
        _v;                                                                                                            \
    })

#endif
