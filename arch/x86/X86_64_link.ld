ENTRY(_start)

PHDRS {
	text PT_LOAD FLAGS(5);          /* R_E */
	data PT_LOAD FLAGS(6);          /* RW_ */
	init PT_LOAD FLAGS(7);          /* RWE */
	note PT_NOTE FLAGS(0);          /* ___ */
}

SECTIONS {
    . = 1M;

#ifdef CONFIG_BOOT_MULTIBOOT2
    .multiboot : {
        *(.multiboot*)
    } :text
#endif

    __init_start = .;

    .init.text : ALIGN(0x1000){
        *(.init.text*)
        *(.init.rodata*)
    } :init

    .init.data : ALIGN(0x1000){
        *(.init.data*)
    } :init

    .init.bss (NOLOAD) : ALIGN(0x1000){
        *(.init.bss*)
    } :init

    __init_end = .;

#ifdef CONFIG_X86_64
    . = 0xFFFFFFFF80000000;
#elif defined(CONFIG_X86_32)
    . = 0xC0000000;
#else
#error "Invalid architecture"
#endif

    __kernel_start = .;

    .text : ALIGN(0x1000) {
        *(.text*)
        *(.rodata*)
    } :text

    .data : ALIGN(0x1000) {
        *(.data*)
    } :data

    .bss (NOLOAD) : ALIGN(0x1000) {
        *(COMMON)
        *(.bss*)
    } :data

    __kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame)
    }
}
