.section .text
.global switch_to
switch_to:
    push    %rbx
    push    %rbp
    push    %r12
    push    %r13
    push    %r14
    push    %r15

    mov     %rsp, 0(%rdi)
    mov     0(%rsi), %rsp

    pop     %r15
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbp
    pop     %rbx

    // HACK: bad hack. We need to re-enable interrupts manually.
    sti

    ret
