#ifndef _ASI_ASM_LINKAGE_H
#define _ASI_ASM_LINKAGE_H

#ifdef __ASSEMBLY__

// clang-format off

.macro __ENTRY name
.global \name
\name:
.endm

#define ENTRY(name) __ENTRY name

#ifdef HAVE_ASM_UNDERSCORE
#define EXT_C(sym) _##sym
#else
#define EXT_C(sym) sym
#endif // HAVE_ASM_UNDERSCORE

// clang-format on

#endif // __ASSEMBLY__
#endif // _ASI_ASM_LINKAGE_H
