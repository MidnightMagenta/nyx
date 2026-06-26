#include <asi/cpu.h>
#include <asi/generated/asm-offsets.h>

.extern __irq_handler

.macro IRQ_ENTRY vector
.global __irq_entry_\vector
__irq_entry_\vector:
    push    $0
    push    $\vector
    jmp     irq_common_entry
.endm

.section .text
irq_common_entry:
    cmpq    $0x08, 24(%rsp)
    je      1f
    swapgs
    lfence
1:  PUSH_REGS
    mov     %rsp, %rdi
    call    __irq_handler
irq_common_exit:
    POP_REGS
    add     $16, %rsp
    cmpq    $0x8, 8(%rsp)
    je      1f
    swapgs
    lfence
1:  iretq

.altmacro
.set i, 32
.rept 224
IRQ_ENTRY %i
.set i, i+1
.endr
.noaltmacro

