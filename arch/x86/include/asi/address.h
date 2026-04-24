#ifndef _ASI_ADDRESS_H
#define _ASI_ADDRESS_H

#include <asi/memory.h>

#define virt_to_phys(v) (v - ARCH_DIRECT_MAP_BASE)
#define phys_to_virt(p) (p + ARCH_DIRECT_MAP_BASE)

#endif
