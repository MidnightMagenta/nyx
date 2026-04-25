#ifndef _ASI_ISR_ENTRY_H
#define _ASI_ISR_ENTRY_H

#define ISR_COMMON_NAME   __isr_entry_common
#define ISR_TABLE_NAME    __isr_entry_table
#define ISR_ENTRY_NAME(n) __isr_entry_##n

#ifdef __ASSEMBLY__
// clang-format off

.macro __isr_entry_no_error name vector common_name
.global \name
\name:
    push $0
    push $\vector
    jmp \common_name
.endm

.macro __isr_entry_error name vector common_name
.global \name
\name:
    push $\vector
    jmp \common_name
.endm

#define ISR_ENTRY_NO_ERR(n) __isr_entry_no_error ISR_ENTRY_NAME(n) n ISR_COMMON_NAME
#define ISR_ENTRY_ERR(n) __isr_entry_error ISR_ENTRY_NAME(n) n ISR_COMMON_NAME

// clang-format on
#else

#include <nyx/compiler.h>
#include <nyx/types.h>

struct idt_data {
    u8  vector;
    u16 segment;
    u8  ist;
    u8  type;
    u8  dpl;
    u16 _pad;
    u64 addr;
} __packed;

#endif

#endif
