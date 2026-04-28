#include <asi/i8259.h>
#include <asi/io.h>

static unsigned int cached_irq_mask = 0xffff;

#define __byte(x, y)       (((unsigned char *) &(y))[x])
#define cached_master_mask (__byte(0, cached_irq_mask))
#define cached_slave_mask  (__byte(0, cached_irq_mask))

void i8259_mask(unsigned int irq) {
    unsigned int mask = 1 << irq;
    cached_irq_mask |= mask;

    if (irq & 8) {
        outb_p(PIC_SLAVE_IMR, cached_slave_mask);
    } else {
        outb_p(PIC_MASTER_IMR, cached_master_mask);
    }
}

void i8259_unmask(unsigned int irq) {
    unsigned int mask = ~(1 << irq);
    cached_irq_mask &= mask;

    if (irq & 8) {
        outb_p(PIC_SLAVE_IMR, cached_slave_mask);
    } else {
        outb_p(PIC_MASTER_IMR, cached_master_mask);
    }
}

void i8259_mask_all() {
    cached_irq_mask = 0xffff;
    outb_p(PIC_MASTER_IMR, cached_master_mask);
    outb_p(PIC_SLAVE_IMR, cached_slave_mask);
}

void i8259_eoi(unsigned int irq);
void i8259_init();
