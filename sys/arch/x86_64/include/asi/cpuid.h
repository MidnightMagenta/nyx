#ifndef _ASI_CPUID_H
#define _ASI_CPUID_H

#include <nyx/types.h>

#define CPUID_LEAF_MANUFACTURER_ID 0x00000000
#define CPUID_LEAF_FEATURES        0x00000001
#define CPUID_LEAF_CACHE_TLB       0x00000002
#define CPUID_LEAF_SERIAL          0x00000003
#define CPUID_LEAF_CACHE_PARAMS    0x00000004
#define CPUID_LEAF_MONITOR         0x00000005
#define CPUID_LEAF_THERMAL_POWER   0x00000006
#define CPUID_LEAF_EXT_FEATURES    0x80000007
#define CPUID_LEAF_XSAVE           0x0000000D
#define CPUID_LEAF_TOPOLOGY_V2     0x0000001F
#define CPUID_LEAF_EXT_MAX         0x80000000
#define CPUID_LEAF_EXT_FEATURES2   0x80000001
#define CPUID_LEAF_BRAND_STRING_0  0x80000002
#define CPUID_LEAF_BRAND_STRING_1  0x80000003
#define CPUID_LEAF_BRAND_STRING_2  0x80000004
#define CPUID_LEAF_ADD_SIZES       0x80000008


static inline void cpuid(u32 leaf, u32 subleaf, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx) {
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "a"(leaf), "c"(subleaf)
                 : "memory");
}

#endif
