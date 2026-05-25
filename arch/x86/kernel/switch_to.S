#include <asi/generated/asm-offsets.h>

.extern finish_context_switch

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
 
    call    finish_context_switch

    ret

.global start_task
start_task:
    mov     %r12, %rdi
    iretq
