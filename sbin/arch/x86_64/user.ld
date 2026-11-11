OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS
{
    text   PT_LOAD FILEHDR PHDRS FLAGS(5);   /* R+X */
    rodata PT_LOAD FLAGS(4);                 /* R   */
    data   PT_LOAD FLAGS(6);                 /* R+W */
    tls    PT_TLS;                           /* template for libc's TLS setup   */
    stack  PT_GNU_STACK FLAGS(6);            /* RW, non-executable stack        */
}

SECTIONS
{
    . = 0x400000 + SIZEOF_HEADERS;

    .text : {
        *(.text.unlikely .text.unlikely.*)
        *(.text.hot .text.hot.*)
        *(.text .text.*)
    } :text
    PROVIDE(etext = .);

    . = ALIGN(4096);
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(4096);
    .tdata : { *(.tdata .tdata.*) } :data :tls
    .tbss  : { *(.tbss .tbss.*) *(.tcommon) } :data :tls

    .init_array : {
        PROVIDE(__init_array_start = .);
        KEEP(*(.init_array .init_array.*))
        PROVIDE(__init_array_end = .);
    } :data
    .fini_array : {
        PROVIDE(__fini_array_start = .);
        KEEP(*(.fini_array .fini_array.*))
        PROVIDE(__fini_array_end = .);
    } :data

    .data : { *(.data .data.*) } :data

    .bss : {
        PROVIDE(__bss_start = .);
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
    } :data
    PROVIDE(_end = .);
    PROVIDE(end = .);

    /DISCARD/ : {
        *(.note .note.*)
        *(.comment)
        *(.eh_frame .eh_frame_hdr .eh_frame.*)
    }
}
