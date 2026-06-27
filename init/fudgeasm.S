bits 64
org 0x10000
section .text
    mov     rax, 57
    syscall
    cmp     rax, 0
    jnz     .Wait
    mov     rax, 60
    mov     rdi, 0
    syscall
.Wait:
    mov rax, 61
    mov rdi, 0xFFFFFFFF
    mov rsi, 0
    mov rdx, 0
    syscall
.Loop:
    jmp     .Loop
