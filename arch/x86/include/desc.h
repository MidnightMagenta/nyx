#ifndef _ASI_DESC_H
#define _ASI_DESC_H

#include <asi/descriptors.h>
#include <nyx/types.h>
#include <string.h>

static inline void load_idt(const struct desc_ptr *ptr) {
    __asm__ volatile("lidt %0" ::"m"(*ptr));
}
static inline void load_gdt(const struct desc_ptr *ptr) {
    __asm__ volatile("lgdt %0" ::"m"(*ptr));
}

static inline void store_idt(struct desc_ptr *ptr) {
    __asm__ volatile("lidt %0" : "=m"(*ptr)::"memory");
}
static inline void store_gdt(struct desc_ptr *ptr) {
    __asm__ volatile("lgdt %0" : "=m"(*ptr)::"memory");
}


static inline void idt_init_desc(gate_desc *gate, const struct idt_data *d) {
    u64 addr = (u64) d->addr;

    gate->offset0  = (u16) addr;
    gate->offset1  = (u16) (addr >> 16);
    gate->offset2  = (u32) (addr >> 32);
    gate->segment  = d->segment;
    gate->bits     = d->bits;
    gate->reserved = 0;
}

static inline void write_idt_entry(gate_desc *idt, int vector, const gate_desc *gate) {
    memcpy(&idt[vector], gate, sizeof(*gate));
}

#endif
