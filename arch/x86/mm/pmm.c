#include <asi/bootparam.h>
#include <asi/page.h>
#include <mm/pmm.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <nyx/util.h>

#define pr_fmt(fmt) ("pm: arch: " fmt "\n")

extern const struct memmap *memmap_get();
extern void                 __do_pm_init(const struct memmap *memmap);

void __init pm_init() {
    __do_pm_init(memmap_get());
}

extern char __image_start;
extern char __image_end;

void __init pm_arch_reserve_regions() {
    int         res;
    phys_addr_t kern_load_base, kern_load_size;

    kern_load_base = bootparams->kernel_load_base;
    kern_load_size = ((u64) &__image_end - (u64) &__image_start);

    if ((res = pm_reserve_region(kern_load_base, ALIGN_UP(kern_load_size, PAGE_SIZE) >> PAGE_SHIFT))) {
        early_panic(pr_fmt("could not reserved kernel"));
    }
}
