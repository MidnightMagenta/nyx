#ifndef _ASI_ADDRESS_H
#define _ASI_ADDRESS_H

#include <asi/memory.h>
#include <nyx/types.h>

#define virt_to_phys(v) (v - ARCH_DIRECT_MAP_BASE)
#define phys_to_virt(p) (p + ARCH_DIRECT_MAP_BASE)

#define __va(a) ((void *) phys_to_virt((phys_addr_t) (a)))
#define __pa(a) ((void *) virt_to_phys((phys_addr_t) (a)))

#endif
