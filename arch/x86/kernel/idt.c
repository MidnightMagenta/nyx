#include <asi/descriptors.h>
#include <nyx/linkage.h>

#define IDT_ENTRIES 256

gate_desc __page_aligned_bss idt[IDT_ENTRIES];
