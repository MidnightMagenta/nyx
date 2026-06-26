#ifndef _ASI_X86_MSR_H
#define _ASI_X86_MSR_H

#include <nyx/types.h>

#define MSR_EFER           0xC0000080
#define MSR_KERNEL_GS_BASE 0xC0000102
#define MSR_GS_BASE        0xC0000101
#define MSR_STAR           0xC0000081
#define MSR_LSTAR          0xC0000082
#define MSR_CSTAR          0xC0000083
#define MSR_SFMASK         0xC0000084

#define EFER_SCE (1 << 0)

static inline u64 rdmsr(u32 msr) {
    u32 lo, hi;
    asm volatile("rdmsr"
                 : "=a"(lo), "=d"(hi)
                 : "c"(msr));
    return ((u64) hi << 32) | lo;
}

static inline void wrmsr(u32 msr, u64 value) {
    u32 lo = value & 0xFFFFFFFF;
    u32 hi = value >> 32;
    asm volatile("wrmsr" ::"a"(lo), "d"(hi), "c"(msr));
}

#endif
