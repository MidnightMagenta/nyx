#include <nyx/linkage.h>
#include <nyx/types.h>

#include <asi/cpufeatures.h>
#include <asi/cpuid.h>
#include <asi/cpuid_bits.h>

struct cpu_features g_cpu_features;

void __init detect_cpu_features() {
    u32 eax, ebx, ecx, edx;

    // Leaf 0x80000001: Extended Feature Information
    cpuid(CPUID_LEAF_EXT_FEATURES2, 0, &eax, &ebx, &ecx, &edx);
    g_cpu_features.cf_pdpe1g = edx & CPUID_80000001_EDX_PDPE1G;
}
