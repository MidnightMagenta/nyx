#include <asi/bootparam.h>
#include <asi/setupdata.h>
#include <mm/memmap.h>
#include <nyx/linkage.h>

extern const struct memmap *memmap_get();
extern void                 __do_pm_init(const struct memmap *memmap);

void __init pm_init() {
    const struct memmap *memmap;

    memmap = memmap_get();
    __do_pm_init(memmap);
}
