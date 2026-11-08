#ifndef _ASI_ISR_H
#define _ASI_ISR_H

#include <nyx/irq.h>
#include <nyx/types.h>

#include <asi/system.h>

unsigned int vector_to_irq(unsigned int vector);

static inline void arch_irq_disable() {
    cli();
}

static inline void arch_irq_enable() {
    sti();
}

static inline flags_t arch_irq_save() {
    flags_t flags;
    asm volatile("pushf; pop %0; cli"
                 : "=r"(flags));
    return flags;
}

static inline void arch_irq_restore(flags_t flags) {
    asm volatile("push %0; popf" ::"r"(flags));
}

#endif
