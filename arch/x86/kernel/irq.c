#include <nyx/linkage.h>

#include <asi/i8259.h>
#include <asi/irq.h>
#include <asi/irq_vectors.h>

void __init arch_init_irq() {
    i8259_init();
}

unsigned int vector_to_irq(unsigned int vector) {
    return vector - FIRST_EXTERNAL_VECTOR;
}

void handle_irq(struct irq_desc *desc) {
    // HACK: we're just doing PIC for now. Obv wrong

    if (i8259_is_spurious(desc->irq)) { goto exit; }

    for (struct list_head *cur = desc->actions.next; cur != &desc->actions; cur = cur->next) {
        struct irq_action *action = list_entry(cur, struct irq_action, list);
        action->fn(action->ctx);
    }

exit:
    i8259_eoi(desc->irq);
}
