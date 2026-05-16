#ifndef _ENTRY_DEFS_H
#define _ENTRY_DEFS_H

#ifdef __ASSEMBLY__
// clang-format off

.macro __ENTRY name
.global \name
\name:
.endm

#define ENTRY(name) __ENTRY name

.macro IDT_DATA_ENTRY vector segment ist type dpl handler
.extern \handler
.byte   \vector
.short  \segment
.byte   \ist
.byte   \type
.byte   \dpl
.short  0
.quad   \handler
.endm

// clang-format on
#endif

#endif
