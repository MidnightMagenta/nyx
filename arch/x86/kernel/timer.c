#include <nyx/irq.h>
#include <nyx/linkage.h>
#include <nyx/printk.h>
#include <nyx/tick.h>

#include <asi/i8253.h>
#include <asi/i8259.h>

extern void tick_handle();

static void timer_irq_handler(void *) {
    tick_handle();
}

void __init arch_init_timer() {
    pit_set_frequency(BASE_TICK_FRQ);
    irq_install_handler(0, timer_irq_handler, "timer", NULL);
    i8259_unmask(0);
}
