.extern idt
.global startup_64

#include <asi/asm_linkage.h>

.section .init.text, "ax"
/*
 * RDI = boot_params pointer
 */
startup_64:
    cli
    mov     %rdi, bootparams(%rip)
    xor     %rbp, %rbp
    leaq    init_stack_top(%rip), %rsp
    pushq   %rbp
    movq    %rsp, %rbp
    call    setup_gdt
    leaq    1f(%rip), %rax
    pushq   $0x08
    pushq   %rax
    retfq

1:  call    setup_idt
    call    EXT_C(start_kernel)

    cli
1:  hlt
    jmp     1b

setup_gdt:
    leaq    gdt(%rip), %rcx
    movq    %rcx, gdt_desc+2(%rip)
    lgdt    gdt_desc(%rip)
    xor     %rax, %rax
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss
    ret

setup_idt:
    leaq    ignore_int(%rip), %rdx
    mov     $0x00080000, %eax
    mov     %dx, %ax
    mov     $0x8E00, %dx
    mov     %edx, %edi
    shl     $32, %rdi
    or      %rdi, %rax
    shr     $32, %rdx

    leaq    idt(%rip), %rdi
    mov     $256, %rcx

1:  mov     %rax, (%rdi)
    mov     %rdx, 8(%rdi)
    add     $16, %rdi
    loop    1b

    leaq    idt(%rip), %rcx
    mov     %rcx, idt_desc+2(%rip)
    lidt    idt_desc(%rip)
    ret

ignore_int:
    iretq

.section    .init.bss, "aw", @nobits
.skip       0x1000
init_stack_top:

.section    .data, "aw"
.align      8
gdt_desc:
    .2byte  256 * 8 - 1
    .8byte  0

.align       8
idt_desc:
    .2byte  256 * 16 - 1
    .8byte  0

.align      0x1000
gdt:
    .8byte  0x0000000000000000
    .8byte  0x00af9a000000ffff
    .8byte  0x00af92000000ffff
    .skip   253 * 8

.section    .init.bss, "aw", @nobits
.global bootparams
bootparams:
    .8byte  0
