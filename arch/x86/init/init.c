#include <nyx/compiler.h>
#include <nyx/early_serial.h>
#include <nyx/linkage.h>
#include <nyx/types.h>

void __init kernel_init(u64 bi) {
    (void) bi;

    early_serial_write("Hello, kernel!\n");
}
