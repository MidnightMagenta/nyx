#ifndef _ASI_X86_GDT_H
#define _ASI_X86_GDT_H

#define GDT_ENTRIES 256

#define KERNEL_CODE_SEGMENT 0x08
#define KERNEL_DATA_SEGMENT 0x10
#define USER_CODE_SEGMENT   0x23
#define USER_DATA_SEGMENT   0x1B
#define TSS_SEGMENT         0x28

#endif
