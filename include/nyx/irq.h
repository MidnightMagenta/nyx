#ifndef _NYX_IRQ_H
#define _NYX_IRQ_H

#include <nyx/list.h>

struct irq_chip {
    void (*enable)();
    void (*disable)();
    void (*mask)(unsigned int irq);
    void (*unmask)(unsigned int irq);
    void (*eoi)(unsigned int irq);
};

typedef void (*irq_handler_func)(void *);

struct irq_action {
    struct list_head list;
    irq_handler_func fn;
    void            *ctx;

    char name[32];
};

struct irq_desc {
    struct list_head actions;
    unsigned int     irq;
};

int irq_install_handler(unsigned int irq, irq_handler_func fn, const char *name, void *ctx);
int irq_remove_handler(unsigned int irq, irq_handler_func fn);

#endif
