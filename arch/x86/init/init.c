#include <asi/boot.h>
#include <nyx/compiler.h>
#include <nyx/early_printk.h>
#include <nyx/linkage.h>
#include <nyx/types.h>
#include <nyx/util.h>

void __init kernel_init(u64 bi) {
    (void) bi;
    early_printk("Hello, kernel!\n");
}
