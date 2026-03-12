#include "boot_utils.h"
#include "printb.h"
#include "region_alloc.h"
#include <nyx/util.h>

void boot_main(unsigned long bi) {
    (void) bi;
    boot_serial_init();
    ra_init(bi);

    hcf();
}
