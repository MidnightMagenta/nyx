#ifndef _ASI_ISR_H
#define _ASI_ISR_H

#include <asi/isr_context.h>
#include <nyx/irq.h>

typedef void (*isr_func_t)(struct isr_context *);

struct __irq_table_ent {
    isr_func_t       fn;
    struct irq_chip *chip;
};

void irq_chip_reserve_range(struct irq_chip *chip, unsigned int from, unsigned int to);
int  irq_register(int vector, isr_func_t fn);
int  irq_deregister(int vector);
void irq_register_interrupts();

#endif
