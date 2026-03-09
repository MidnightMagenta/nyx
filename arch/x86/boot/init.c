#include <nyx/types.h>

#define hcf()                                                                                                          \
    while (1) { __asm__ volatile("hlt"); }

extern int boot_serial_init();
extern int printb(const char *fmt, ...);

void boot_main(u64 bi) {
    (void) bi;
    boot_serial_init();
    printb("Hello, boot stub\n");
    hcf();
}
