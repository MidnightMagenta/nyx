#include <nyx/errno.h>
#include <nyx/stddef.h>

#include <asi/i8259.h>
#include <asi/idt.h>
#include <asi/irq.h>

struct __irq_table_ent isr_fn_arr[IDT_ENTRIES] = {0};

void irq_chip_reserve_range(struct irq_chip *chip, unsigned int from, unsigned int to) {
    for (size_t i = from; i < to; i++) { isr_fn_arr[to].chip = chip; }
}

int irq_register(int vector, isr_func_t fn) {
    if ((unsigned int) vector > IDT_ENTRIES) { return -EINVAL; }
    if (!isr_fn_arr[vector].chip) { return -ENODEV; }
    if (isr_fn_arr[vector].fn) { return -EBUSY; }
    isr_fn_arr[vector].fn = fn;
    return 0;
}

int irq_deregister(int vector) {
    if ((unsigned int) vector > IDT_ENTRIES) { return -EINVAL; }
    isr_fn_arr[vector].fn = NULL;
    return 0;
}

void irq_register_interrupts() {}
