.code32
.section .multiboot, "a"

#include "multiboot2.h"

.align 8
mb_header:
    .int    MULTIBOOT2_HEADER_MAGIC
    .int    MULTIBOOT2_ARCH_i386
    .int    (mb_header_end - mb_header)
    .int    -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCH_i386 + (mb_header_end - mb_header))

mb_entry_address_tag:
    .short  MULTIBOOT2_TAG_ADDRESS
    .short  0
    .int    (mb_entry_address_tag_end - mb_entry_address_tag)
    .int    _start
mb_entry_address_tag_end:

    // end tag
    .short  0
    .short  0
    .int    8
mb_header_end:

.section .init.text, "ax"

.global start, _start

start:
_start:
    cli
    cmpl    $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
    jne     bad_boot
    mov     $boot_stack_top, %esp
    push    %eax    // preserve multiboot magic
    push    %ebx    // preserve boot info poiner

    // verify the CPU supports long mode
    mov     $0x80000000, %eax
    cpuid
    cmp     $0x80000000, %eax
    jbe     no_long_mode
    mov     $0x80000001, %eax
    cpuid
    bt      $29, %edx
    jnc     no_long_mode

    // long mode is available
    // identity map the first 64 MiB of memory

    // ensure the page tables are zeroed out
    cld
    // zero out the PD
    mov     $1024, %ecx
    mov     $pd, %edi
    xor     %eax, %eax
    rep stosl

    // zero out the PDPT
    mov     $1024, %ecx
    mov     $pdpt, %edi
    rep stosl

    // zero out the PML4
    mov     $1024, %ecx
    mov     $pml4, %edi
    rep stosl

    // map 64 MiB using 2 MiB pages in the PD
    mov     $32, %ecx
    mov     $pd, %edi
    xor     %eax, %eax
1:
    movl    %eax, (%edi)    // write low 32 bits
    orl     $0x83, (%edi)   // P=1, RW=1, PS=1
    movl    $0, 4(%edi)     // zero high 32 bits

    add     $8, %edi
    add     $0x200000, %eax
    loop    1b

    // set entry 0 in PDPT to PD
    mov     $pd, %eax
    or      $0x3, %eax
    mov     $pdpt, %edi
    movl    %eax, (%edi)
    movl    $0, 4(%edi)

    // set entry 0 in PML4 to PDPT
    mov     $pdpt, %eax
    or      $0x3, %eax
    mov     $pml4, %edi
    movl    %eax, (%edi)
    movl    $0, 4(%edi)

    // load a GDT containing a 64 bit code and data segment descriotors
    lgdt    gdt32_desc

    // set CR4.PAE 
    mov     %cr4, %eax
    bts     $5, %eax
    mov     %eax, %cr4

    // load the PML4 address into CR3
    mov     $pml4, %eax
    mov     %eax, %cr3

    // set EFER.LME
    mov     $0xC0000080, %ecx
    rdmsr
    bts     $8, %eax
    wrmsr

    // set CR0.PG
    mov     %cr0, %eax
    bts     $31, %eax
    mov     %eax, %cr0

    // jump to long mode
    pop     %ebx
    pop     %eax
    jmp    $0x08, $long_mode

no_long_mode:
bad_boot:
    hlt
    jmp     bad_boot

.section .init.data, "aw"
gdt32_desc:
.word   gdt32_end - gdt32 - 1
.int    gdt32

.align 8
gdt32:
.quad 0x0000000000000000
.quad 0x00209a0000000000    // kernel code
.quad 0x0000920000000000    // kernel data
gdt32_end:

// TODO: boot IDT

.section .init.bss, "aw", @nobits
.align  0x1000
pml4:
.skip   0x1000

.align  0x1000
pdpt:
.skip   0x1000

.align  0x1000
pd:
.skip   0x1000

.align  0x1000
boot_stack_bottom:
.skip   1024
boot_stack_top:

.code64
.section .init.text, "ax"
long_mode:
    mov     %rax, %r15
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss
    mov     $boot_stack_top, %rsp
    // for now halt and catch fire
1:  hlt
    jmp     1b
