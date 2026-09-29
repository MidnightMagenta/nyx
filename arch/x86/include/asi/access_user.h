#ifndef _X86_ASI_ACCESS_USER_H
#define _X86_ASI_ACCESS_USER_H

#include <asi/exception.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

extern size_t  __copy_user(void *dst, const void *src, size_t len);
extern ssize_t __copyinstr(char *dst, const char *usrc, size_t max);

static inline int get_user_u8(u8 *out, const u8 *uptr) {
    int err = 0;
    u8  v;
    asm volatile("1: movb (%[p]), %[v]\n"
                 "2: \n"
                 ".pushsection .text.fixup, \"ax\"\n"
                 "3: movl $-14, %[e]\n"
                 "   jmp 2b\n"
                 ".popsection\n" EXTABLE_ENTRY(1b, 3b)
                 : [e] "+r"(err), [v] "=q"(v)
                 : [p] "r"(uptr)
                 : "memory");
    *out = v;
    return err;
}

#endif
