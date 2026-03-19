#include <nyx/linkage.h>

extern const struct memmap *memmap_get();
extern void                 __do_pm_init(const struct memmap *memmap);

void __init pm_init() {
    __do_pm_init(memmap_get());
}
