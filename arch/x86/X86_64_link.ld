ENTRY(_start)

PHDRS {
	init PT_LOAD FLAGS(7);          /* RWE */
	text PT_LOAD FLAGS(5);          /* R_E */
	data PT_LOAD FLAGS(6);          /* RW_ */
	note PT_NOTE FLAGS(0);          /* ___ */
}

SECTIONS {
    . = 1M;
    __init_start = .;

#ifdef CONFIG_BOOT_MULTIBOOT2
    .multiboot BLOCK(8) : AT(1M) {
        *(.multiboot*)
    } :init
#endif

    .init.text BLOCK(0x1000) : ALIGN(0x1000) {
        *(.init.text*)
        *(.init.rodata*)
    } :init

    .init.data BLOCK(0x1000) : ALIGN(0x1000) {
        *(.init.data*)
    } :init

    .init.bss (NOLOAD) : ALIGN(0x1000) {
        *(.init.bss*)
    } :init

    __init_end = .;

#ifdef CONFIG_X86_64
    . = 0xFFFFFFFF80000000;
#elif defined(CONFIG_X86_32)
    . = 0xC0000000;
#else
#error "Invalid processor family"
#endif

    __kernel_start = .;

    .text BLOCK(0x1000) : ALIGN(0x1000) {
        *(.text*)
        *(.rodata*)
    } :text

    .data BLOCK(0x1000) : ALIGN(0x1000) {
        *(.data*)
    } :data

    .bss BLOCK(0x1000) (NOLOAD) : ALIGN(0x1000) {
        *(COMMON)
        *(.bss*)
    } :data

    __kernel_end = .;

    /DISCARD/ : {
        *(.note.gnu.property)
        *(.eh_frame)
    }
}
