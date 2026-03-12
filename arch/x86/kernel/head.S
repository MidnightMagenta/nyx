.global startup_64

#include <asi/asm_linkage.h>

.section .init.text, "ax"
/*
 * RDI = boot_params pointer
 */
startup_64:
    cli
    mov $init_stack_top, %rsp
    call EXT_C(start_kernel)
1:  hlt
    jmp 1b

.section .init.bss, "aw", @nobits
.skip 0x1000
init_stack_top:
