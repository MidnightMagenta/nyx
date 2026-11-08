#ifndef _X86_ASI_TSC_H
#define _X86_ASI_TSC_H

#include <nyx/types.h>

static inline u64 read_tsc() {
    u32 tsc_lo, tsc_hi;

    asm volatile("rdtsc"
                 : "=a"(tsc_lo), "=d"(tsc_hi)::"memory");

    return (u64) ((((u64) tsc_hi) << 32) | (u64) tsc_lo);
}

#endif
