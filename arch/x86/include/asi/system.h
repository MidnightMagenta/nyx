#ifndef _ASI_SYSTEM_H
#define _ASI_SYSTEM_H

#define cli() __asm__ volatile("cli");
#define hlt() __asm__ volatile("hlt");
#define invlpg(a)                                                                                                      \
    __asm__ volatile("invlpg (%0)"                                                                                     \
                     :                                                                                                 \
                     : "r"(a)                                                                                          \
                     : "memory")

#define hcf()                                                                                                          \
    cli();                                                                                                             \
    while (1) { hlt(); }

#endif
