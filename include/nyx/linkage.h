#ifndef _NYX_LINKAGE_H
#define _NYX_LINKAGE_H

#include <asi/page.h>
#include <nyx/compiler.h>

#define __page_aligned_bss __section(".bss") __align(PAGE_SIZE)

#define __boot     __section(".boot.text")
#define __bootdata __section(".boot.data")
#define __bootzero __section(".boot.bss")

#define __init      __section(".init.text")
#define __initdata  __section(".init.data")
#define __initconst __section(".init.rodata")
#define __initzero  __section(".init.bss")

#endif
