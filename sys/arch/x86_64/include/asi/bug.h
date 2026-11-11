#ifndef _ASI_BUG_H
#define _ASI_BUG_H

#define ASM_UD2 ".byte 0x0f, 0x0b"

#define BUG() asm volatile(ASM_UD2)

#define BUG_ON(cond)                                                                                                   \
    do {                                                                                                               \
        if (!!(cond)) { BUG(); }                                                                                       \
    } while (0)

#endif
