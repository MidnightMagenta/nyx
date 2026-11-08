#include <nyx/tick.h>

volatile tick_t jiffies;

extern void arch_init_timer();

void init_timer() {
    arch_init_timer();
}

extern void scheduler_tick();

void tick_handle() {
    jiffies++;
    scheduler_tick();
}
