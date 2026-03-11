ENTRY(_start)

PHDRS {
    text        PT_LOAD FLAGS(5);           /* R_E */
    data        PT_LOAD FLAGS(6);           /* RW_ */
    blob        PT_LOAD FLAGS(6);           /* RW_ */
    note        PT_NOTE FLAGS(0);           /* ___ */
}

SECTIONS {
    . = 1M;
 
    .text BLOCK(0x1000) : {
        KEEP(*(.multiboot*))
        *(.text*)
        *(.rodata*)
    } :text
    
    .data BLOCK(0x1000) : {
        *(.data)
    } :data

    .bss BLOCK(0x1000) (NOLOAD) : {
        *(COMMON)
        *(.bss)
    } :data

    .kernel_blob BLOCK(0x1000) : {
        __kernel_blob_start = .;
        KEEP(*(.kernel_blob*))
        __kernel_blob_end = .;
    } :blob

    __kernel_blob_size = __kernel_blob_end - __kernel_blob_start;

    /DISCARD/ : {
        *(.note.gnu.property)
        *(.eh_frame)
    }
}
