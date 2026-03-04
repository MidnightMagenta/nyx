#! /bin/sh
set -euo pipefail

qemu-system-x86_64 -cdrom nyxos.iso -m 2G -cpu qemu64 -vga std -d int,guest_errors,cpu_reset -no-reboot -no-shutdown
