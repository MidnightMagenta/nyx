ENTRY(startup_64)

PHDRS {
    init_text   PT_LOAD FLAGS(5);          /* R_E */
    init_data   PT_LOAD FLAGS(6);          /* RW_ */
    text        PT_LOAD FLAGS(5);          /* R_E */
    data        PT_LOAD FLAGS(6);          /* RW_ */
/*  note        PT_NOTE FLAGS(0);          /* ___ */
}

#define ARCH_KERNEL_BASE 0xFFFFFFFF80000000

SECTIONS {
    . = ARCH_KERNEL_BASE;
    __image_virt_start = .;
    __kernel_virt_start = .;

    .text BLOCK(0x1000) : {
        __text_virt_start = .;
        *(.text*)
        __text_virt_end = .;
    } :text

    .rodata BLOCK(0x1000) : {
        __rodata_virt_start = .;
        *(.rodata*)
        __rodata_virt_end = .;
    } :text

    .data BLOCK(0x1000) : {
        __data_virt_start = .;
        *(.data*)
    } :data
 
    .bss BLOCK(0x1000) (NOLOAD) : {
        *(COMMON)
        *(.bss*)
        __data_virt_end = .;
    } :data
 
    __kernel_virt_end = .;

    __init_virt_start = .;

    .init.text BLOCK(0x1000) : {
        *(.init.text*)
        *(.init.rodata*)
    } :init_text

    .init.data BLOCK(0x1000) : {
        *(.init.data*)
    } :init_data

    .init.bss BLOCK(0x1000) (NOLOAD) : {
        *(.init.bss*)
    } :init_data

    __init_virt_end = .;
    
    __image_virt_end = .;

    /DISCARD/ : {
        *(.note.gnu.property)
        *(.eh_frame)
    }
}
