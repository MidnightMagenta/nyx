#include <nyx/tick.h>

volatile tick_t jiffies;

extern void arch_init_timer();

void init_timer() {
    arch_init_timer();
}

void tick_handle() {
    jiffies++;
}
