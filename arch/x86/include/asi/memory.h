#ifndef _ASI_MEMORY_H
#define _ASI_MEMORY_H

#define ARCH_DIRECT_MAP_BASE 0xFFFF888000000000ULL
#define ARCH_KERNEL_BASE     0xFFFFFFFF80000000ULL

#define virt_to_phys(v) (v - ARCH_DIRECT_MAP_BASE)
#define phys_to_virt(p) (p + ARCH_DIRECT_MAP_BASE)

#endif
