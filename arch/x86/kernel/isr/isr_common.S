#include <asi/isr_entry.h>
#include <asi/cpu.h>

.section .text, "ax"

.extern __isr_dispatch

.global ISR_COMMON_NAME
ISR_COMMON_NAME:
    PUSH_REGS
    mov     %rsp, %r15              // save the rsp before we align it for the call (r15 is callee saved)
    mov     %rsp, %rdi              // argument to __isr_dispatch
    and     $-16, %rsp              // align rsp per ABI
    sub     $8, %rsp
    call    __isr_dispatch
    mov     %r15, %rsp              // restore old rsp
    POP_REGS
    add     $16, %rsp               // pop error code and vector number
    iretq
