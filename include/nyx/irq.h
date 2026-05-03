#ifndef _NYX_IRQ_H
#define _NYX_IRQ_H

struct irq_chip {
    void (*enable)();
    void (*disable)();
    void (*mask)(unsigned int irq);
    void (*unmask)(unsigned int irq);
    void (*eoi)(unsigned int irq);
};

#endif
