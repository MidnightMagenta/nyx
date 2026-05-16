#ifndef _NYX_TICK_H
#define _NYX_TICK_H

#include <nyx/types.h>

#define BASE_TICK_FRQ 250

typedef u64 tick_t;

extern volatile tick_t jiffies;

static inline u64 ticks_to_ms(tick_t t) {
    return t * (1000 / BASE_TICK_FRQ);
}
static inline u64 uptime_ms() {
    return ticks_to_ms(jiffies);
}

#endif
