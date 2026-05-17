#include <asi/i8259.h>
#include <asi/io.h>
#include <asi/irq_vectors.h>
#include <nyx/irq.h>
#include <nyx/types.h>

#define ICW1_NEED_ICW4 (1 << 0)
#define ICW1_SINGLE    (1 << 1)
#define ICW1_INIT      (1 << 4)
#define DEFAULT_ICW1   (ICW1_INIT | ICW1_NEED_ICW4)

#define ICW4_8086    (1 << 0)
#define ICW4_AEOI    (1 << 1)
#define DEFAULT_ICW4 (ICW4_8086)

#define OCW3     0x08
#define OCW3_RR  (1 << 1)
#define OCW3_ISR (1 << 0)

#define CASCADE_IRQ 2

static unsigned int cached_irq_mask = 0xffff;

#define __byte(x, y)       (((unsigned char *) &(y))[x])
#define cached_master_mask (__byte(0, cached_irq_mask))
#define cached_slave_mask  (__byte(0, cached_irq_mask))

void i8259_mask(unsigned int irq) {
    unsigned int mask = 1 << (irq);
    cached_irq_mask |= mask;

    if (irq & 8) {
        outb_p(PIC_SLAVE_IMR, cached_slave_mask);
    } else {
        outb_p(PIC_MASTER_IMR, cached_master_mask);
    }
}

void i8259_unmask(unsigned int irq) {
    unsigned int mask = ~(1 << (irq));
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

int i8259_is_spurious(unsigned int irq) {
    u16 port     = ((irq) < 8) ? PIC_MASTER_CMD : PIC_SLAVE_CMD;
    u8  irq_mask = 1 << ((irq) & 7);
    u8  isr;

    outb_p(port, OCW3 | OCW3_RR | OCW3_ISR);
    isr = inb_p(port);

    return !(isr & irq_mask);
}

void i8259_eoi(unsigned int irq) {
    unsigned int pic_irq = irq;
    if (i8259_is_spurious(pic_irq)) { goto handle_spurious; }

    if (pic_irq > 7) { outb_p(PIC_SLAVE_CMD, 0x20); }
    outb_p(PIC_MASTER_CMD, 0x20);
    return;

handle_spurious:
    if (pic_irq > 7) { outb_p(PIC_MASTER_CMD, 0x20); }
}

void i8259_init() {
    outb_p(PIC_MASTER_IMR, 0xff);

    outb_p(PIC_MASTER_CMD, DEFAULT_ICW1);
    outb_p(PIC_SLAVE_CMD, DEFAULT_ICW1);

    outb_p(PIC_MASTER_IMR, FIRST_EXTERNAL_VECTOR);
    outb_p(PIC_SLAVE_CMD, FIRST_EXTERNAL_VECTOR + 8);

    outb_p(PIC_MASTER_IMR, 1 << CASCADE_IRQ);
    outb_p(PIC_SLAVE_IMR, 2);

    outb_p(PIC_MASTER_IMR, DEFAULT_ICW4);
    outb_p(PIC_SLAVE_CMD, DEFAULT_ICW4);

    outb_p(PIC_MASTER_IMR, cached_master_mask);
    outb_p(PIC_SLAVE_IMR, cached_slave_mask);
}

struct irq_chip i8259_chip = {
        .enable  = i8259_init,
        .disable = i8259_mask_all,
        .mask    = i8259_mask,
        .unmask  = i8259_unmask,
        .eoi     = i8259_eoi,
};
