#ifndef _X86_ASI_ACCESS_USER_H
#define _X86_ASI_ACCESS_USER_H

#include <asi/exception.h>
#include <nyx/errno.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

extern size_t  __copy_user(void *dst, const void *src, size_t len);
extern ssize_t __copyinstr(char *dst, const char *usrc, size_t max);

#define DEFINE_GET_USER(name, type, insn)                                                                              \
    static inline int name(type *out, const type *uptr) {                                                              \
        int  err = 0;                                                                                                  \
        type v;                                                                                                        \
        asm volatile("1: " insn " (%[p]), %[v]\n"                                                                      \
                     "2: \n"                                                                                           \
                     ".pushsection .text.fixup, \"ax\"\n"                                                              \
                     "3: movl %[efault], %[e]\n"                                                                       \
                     "   xorl %[v], %[v]\n"                                                                            \
                     "   jmp 2b\n"                                                                                     \
                     ".popsection\n" EXTABLE_ENTRY(1b, 3b)                                                             \
                     : [e] "+r"(err), [v] "=q"(v)                                                                      \
                     : [p] "r"(uptr), [efault] "i"(-EFAULT)                                                            \
                     : "memory");                                                                                      \
        *out = v;                                                                                                      \
        return err;                                                                                                    \
    }

#define DEFINE_WRITE_USER(name, type, insn)                                                                            \
    static inline int name(type v, type *uptr) {                                                                       \
        int err = 0;                                                                                                   \
        asm volatile("1: " insn " %[v], (%[p])\n"                                                                      \
                     "2: \n"                                                                                           \
                     ".pushsection .text.fixup, \"ax\"\n"                                                              \
                     "3: movl %[efault], %[e]\n"                                                                       \
                     "   jmp 2b\n"                                                                                     \
                     ".popsection\n" EXTABLE_ENTRY(1b, 3b)                                                             \
                     : [e] "+r"(err)                                                                                   \
                     : [p] "r"(uptr), [v] "q"(v), [efault] "i"(-EFAULT)                                                \
                     : "memory");                                                                                      \
        return err;                                                                                                    \
    }

DEFINE_GET_USER(__get_user_u8, u8, "movb")
DEFINE_GET_USER(__get_user_u16, u16, "movw")
DEFINE_GET_USER(__get_user_u32, u32, "movl")
DEFINE_GET_USER(__get_user_u64, u64, "movq")

DEFINE_WRITE_USER(__write_user_u8, u8, "movb")
DEFINE_WRITE_USER(__write_user_u16, u16, "movw")
DEFINE_WRITE_USER(__write_user_u32, u32, "movl")
DEFINE_WRITE_USER(__write_user_u64, u64, "movq")

#endif
