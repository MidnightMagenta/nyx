ENTRY(_start)

PHDRS {
    boot        PT_LOAD FLAGS(7);          /* RWE */
    init_text   PT_LOAD FLAGS(5);          /* R_E */
    init_data   PT_LOAD FLAGS(6);          /* RW_ */
    text        PT_LOAD FLAGS(5);          /* R_E */
    data        PT_LOAD FLAGS(6);          /* RW_ */
    note        PT_NOTE FLAGS(0);          /* ___ */
}

#define PHYS_ADVANCE(sec, align)    \
    __phys += SIZEOF(sec);          \
    __phys = ALIGN(__phys, align);

SECTIONS {
    . = 1M;
    __image_phys_start = .;
    __image_virt_start = .;

    __boot_phys_start = .;
    __boot_virt_start = .;

#ifdef CONFIG_BOOT_MULTIBOOT2
    .boot.multiboot BLOCK(8) : {
        *(.boot.multiboot*)
    } :boot
#endif

    .boot.text BLOCK(0x1000) : {
        *(.boot.text*)
    } :boot

    .boot.data BLOCK(0x1000) : {
        *(.boot.data*)
    } :boot

    .boot.bss BLOCK(0x1000) (NOLOAD) : {
        *(.boot.bss*)
    } :boot

    __boot_phys_end = .;
    __boot_virt_end = .;
    __phys = ALIGN(ADDR(.boot.bss) + SIZEOF(.boot.bss), 0x1000);

#ifdef CONFIG_X86_64
    . = 0xFFFFFFFF80000000;
#elif defined(CONFIG_X86_32)
    . = 0xC0000000;
#else
#error "Invalid processor family"
#endif

    __kernel_phys_start = __phys;
    __kernel_virt_start = .;

    __text_phys_start = __phys;

    .text BLOCK(0x1000) : AT(__phys) {
        __text_virt_start = .;
        *(.text*)
        __text_virt_end = .;
    } :text
    PHYS_ADVANCE(.text, 0x1000)

    __text_phys_end = __phys;

    __rodata_phys_start = __phys;

    .rodata BLOCK(0x1000) : AT(__phys) {
        __rodata_virt_start = .;
        *(.rodata*)
        __rodata_virt_end = .;
    } :text
    PHYS_ADVANCE(.rodata, 0x1000)

    __rodata_phys_end = __phys;

    __data_phys_start = __phys;

    .data BLOCK(0x1000) : AT(__phys) {
        __data_virt_start = .;
        *(.data*)
    } :data
    PHYS_ADVANCE(.data, 0x1000)

    .bss BLOCK(0x1000) (NOLOAD) : {
        *(COMMON)
        *(.bss*)
        __data_virt_end = .;
    } :data
    PHYS_ADVANCE(.bss, 0x1000)

    __data_phys_end = __phys;

    __kernel_phys_end = __phys;
    __kernel_virt_end = .;

    __init_phys_start = __phys;
    __init_virt_start = .;

    .init.text BLOCK(0x1000) : AT(__phys) {
        *(.init.text*)
        *(.init.rodata*)
    } :init_text
    PHYS_ADVANCE(.init.text, 0x1000)

    .init.data BLOCK(0x1000) : AT(__phys) {
        *(.init.data*)
    } :init_data
    PHYS_ADVANCE(.init.data, 0x1000)

    .init.bss (NOLOAD) : {
        *(.init.bss*)
    } :init_data
    PHYS_ADVANCE(.init.bss, 0x1000)

    __init_phys_end = __phys;
    __init_virt_end = .;
    
    __image_phys_end = __phys;
    __image_virt_end = .;
    /DISCARD/ : {
        *(.note.gnu.property)
        *(.eh_frame)
    }
}
