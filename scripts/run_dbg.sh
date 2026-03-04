#! /bin/sh
set -euo pipefail

mkdir -p tmp
qemu-system-x86_64 -cdrom nyxos.iso -m 2G -cpu qemu64 -vga std -s -S -d int,guest_errors,cpu_reset -no-reboot -no-shutdown -D tmp/qemu.log &
gdb -ex "symbol-file nyxos" -ex "target remote localhost:1234" -ex "set step-mode on"
