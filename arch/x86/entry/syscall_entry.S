#include <asi/cpu.h>
#include <asi/gdt.h>
#include <asi/generated/asm-offsets.h>

.extern __syscall_handler

.section .text

.global syscall_entry
syscall_entry:
    swapgs
    lfence
    mov     %rsp, %gs:URSP
    mov     %gs:RSP0, %rsp
    pushq   $USER_DATA_SEGMENT
    pushq   %gs:URSP
    pushq   %r11
    pushq   $USER_CODE64_SEGMENT
    pushq   %rcx
    pushq   $0
    pushq   $0xFFFFFFFFFFFFFFFF     // fake vector
    PUSH_REGS
    mov     %rsp, %rdi
    sti
    call    __syscall_handler
.global syscall_exit
syscall_exit:
    POP_REGS
    add     $16, %rsp
    movq    0(%rsp), %rcx
    movq    16(%rsp), %r11
    mov     24(%rsp), %rsp
    cli
    swapgs
    lfence
    sysretq
