#ifndef _ASI_CPU_H
#define _ASI_CPU_H

#ifndef __ASSEMBLY__

#include <nyx/compiler.h>
#include <nyx/types.h>

struct gp_regs {
    u64 rax, rbx, rcx, rdx;
    u64 rsi, rdi, rsp, rbp;
    u64 r8, r9, r10, r11;
    u64 r12, r13, r14, r15;
} __packed;

#else
// clang-format off

.macro PUSH_REGS
    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rbp
    sub  $8, %rsp
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %rbx
    push %rax
.endm

.macro POP_REGS
    pop %rax
    pop %rbx
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    add $8, %rsp
    pop %rbp
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
    pop %r13
    pop %r14
    pop %r15
.endm

// clang-format on
#endif
#endif
