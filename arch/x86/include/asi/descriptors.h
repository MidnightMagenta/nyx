#ifndef _ASI_DESCRIPTORS_H
#define _ASI_DESCRIPTORS_H

#ifndef __ASSEMBLY__

#include <nyx/compiler.h>
#include <nyx/types.h>

struct desc_ptr {
    u16 size;
    u64 base;
} __packed;

struct idt_bits {
    u16 ist : 3, zero : 5, type : 5, dpl : 2, p : 1;
} __packed;

struct gate_struct {
    u16             offset0;
    u16             segment;
    struct idt_bits bits;
    u16             offset1;
    u32             offset2;
    u32             reserved;
} __packed;

typedef struct gate_struct gate_desc;

#endif

#define GATE_INTERRUPT 0xe
#define GATE_TRAP      0xf
#define GATE_CALL      0xc
#define GATE_TASK      0x5

#endif
