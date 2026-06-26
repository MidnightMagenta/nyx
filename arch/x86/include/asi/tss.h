#ifndef _ASI_X86_TSS_H
#define _ASI_X86_TSS_H

#include <nyx/compiler.h>
#include <nyx/types.h>

struct tss {
    u32 reserved0;
    u64 rsp[3];
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iopb_offset;
} __packed;

extern struct tss default_tss;

#endif
