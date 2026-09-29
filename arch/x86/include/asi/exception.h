#ifndef _X86_ASI_EXCEPTION_H
#define _X86_ASI_EXCEPTION_H

#ifdef __ASSEMBLY__

// clang-format off

.macro EXTABLE_ENTRY inst_label, fixup_label
    .pushsection .ex_table, "a"
    .balign 8
    .quad \inst_label
    .quad \fixup_label
    .popsection
.endm

// clang-format on

#else

#include <nyx/types.h>

struct exception_entry {
    uintptr_t insn_addr;
    uintptr_t fixup_addr;
};

#define EXTABLE_ENTRY(insn_label, fixup_label)                                                                         \
    ".pushsection .ex_table, \"a\"\n"                                                                                  \
    "   .balign 8\n"                                                                                                   \
    "   .quad " #insn_label "\n"                                                                                       \
    "   .quad " #fixup_label "\n"                                                                                      \
    ".popsection\n"
#endif

#endif
