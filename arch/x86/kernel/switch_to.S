#include <asi/generated/asm-offsets.h>
#include <asi/cpu.h>

.extern schedule_tail

.section .text
.global switch_to
switch_to:
    push    %rbx
    push    %rbp
    push    %r12
    push    %r13
    push    %r14
    push    %r15

    mov     %rsp, TASK_CONTEXT_RSP(%rdi)
    mov     TASK_CONTEXT_RSP(%rsi), %rsp

    pop     %r15
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbp
    pop     %rbx
 
    ret

.global proc_trampoline
proc_trampoline:
    sti
    call schedule_tail
    mov     %r13, %rdi
    call    *%r12
    POP_REGS
    add     $16, %rsp
    swapgs
    lfence
    iretq
1:  hlt
    jmp     1b
