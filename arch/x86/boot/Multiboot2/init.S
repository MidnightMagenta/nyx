.section .multiboot
.align 8

mb2_header_start:
    .int 0xE85250D6                                                 // magic
    .int 0                                                          // architecture (0 = i386)
    .int mb2_header_end - mb2_header_start                          // header length
    .int -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))    // checksum -(magic + architecture + header length)

    // entry address tag
    .short 3    // type (3 = entry address tag)
    .short 0    // flags
    .int 12     // size
    .int _start // entry address

    // end tag
    .short 0
    .short 0
    .int 8
mb2_header_end:

.section .init.text
.global _start
_start:
    hlt
    jmp _start
