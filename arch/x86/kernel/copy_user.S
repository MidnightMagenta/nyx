#include <asi/exception.h>
#include <asi/asm_linkage.h>
#include <nyx/errno.h>

// TODO: clac/stac
ENTRY(__copy_user)
    movq    %rdx, %rcx
1:  rep movsb
    movq    %rcx, %rax
    ret
2:  movq %rcx, %rax
    ret
    EXTABLE_ENTRY 1b, 2b

ENTRY(__copyinstr)
    testq   %rdx, %rdx
    jz      .Lexhausted
    xorl    %ecx, %ecx
1:  movb    (%rsi, %rcx), %al
    movb    %al, (%rdi, %rcx)
    incq    %rcx
    testb   %al, %al
    jz      .Lfound
    cmpq    %rdx, %rcx
    jb      1b
.Lexhausted:
    xorl    %eax, %eax
    ret
.Lfound:
    movq %rcx, %rax
    ret
2:  movq $-EFAULT, %rax
    ret
    EXTABLE_ENTRY 1b, 2b
