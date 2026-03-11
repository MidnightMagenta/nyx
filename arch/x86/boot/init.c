#include "boot_utils.h"
#include "printb.h"
#include "region_alloc.h"
#include <nyx/util.h>

void boot_main(unsigned long bi) {
    (void) bi;
    boot_serial_init();
    ra_init(bi);
    ra_dump_regions();

    struct memregion test_region = ra_get_region(1024, 0x1000);
    printb("Allocated region [0x%lx, 0x%lx]\n", test_region.base, test_region.base + test_region.size);

    ra_dump_regions();

    hcf();
}
