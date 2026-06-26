bits 64
org 0x10000
section .text
    mov     rax, 0
    syscall
    cmp     rax, 0
    jnz     .Loop
    mov     rax, 1
    mov     rdi, 0
    syscall
.Loop:
    jmp     .Loop
