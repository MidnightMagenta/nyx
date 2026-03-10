#include "elf.h"

#define hcf()                                                                                                          \
    while (1) { __asm__ volatile("hlt"); }

extern int boot_serial_init();
extern int printb(const char *fmt, ...);

extern char __kernel_blob_start;
extern char __kernel_blob_end;
extern char __kernel_blob_size;

void         *kernel_blob_start = &__kernel_blob_start;
void         *kernel_blob_end   = &__kernel_blob_end;
unsigned long kernrel_blob_size = (unsigned long) &__kernel_blob_size;

int bmemcmp(const void *a, const void *b, unsigned long n) {
    const unsigned char *aptr = (const unsigned char *) a;
    const unsigned char *bptr = (const unsigned char *) b;
    while (n--) {
        if (*aptr != *bptr) { return *aptr - *bptr; }
        aptr++;
        bptr++;
    }
    return 0;
}

void boot_main(unsigned long bi) {
    (void) bi;
    boot_serial_init();
    hcf();
}
