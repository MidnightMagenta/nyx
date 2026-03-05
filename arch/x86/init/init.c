#include <nyx/compiler.h>
#include <nyx/early_printk.h>
#include <nyx/linkage.h>
#include <nyx/types.h>

void __init kernel_init(u64 bi) {
    (void) bi;
}
