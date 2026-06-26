#ifndef _ASI_DESCRIPTORS_H
#define _ASI_DESCRIPTORS_H

#ifndef __ASSEMBLY__

#include <nyx/compiler.h>
#include <nyx/types.h>

struct desc_ptr {
    u16 size;
    u64 base;
} __packed;

struct segment_desc {
    u64 __void;
};

struct ssd_bits {
    u16 type   : 4;
    u16 zero0  : 1;
    u16 dpl    : 2;
    u16 p      : 1;
    u16 limit1 : 4;
    u16 avl    : 1;
    u16 zero1  : 2;
    u16 g      : 1;
} __packed;

struct ssd_data {
    u16             limit0;
    u16             base0;
    u8              base1;
    struct ssd_bits bits;
    u8              base2;
    u32             base3;
    u32             zero;
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

struct idt_data {
    u8  vector;
    u16 segment;
    u8  ist;
    u8  type;
    u8  dpl;
    u16 _pad;
    u64 addr;
} __packed;

#endif

#define SEG_TSS 0x9

#define GATE_INTERRUPT 0xe
#define GATE_TRAP      0xf
#define GATE_CALL      0xc
#define GATE_TASK      0x5

#define DPL0 0x0
#define DPL3 0x3

#endif
