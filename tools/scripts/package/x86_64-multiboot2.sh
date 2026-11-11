#!/usr/bin/env bash
# Package a multiboot2 image and its initramfs into a bootable GRUB ISO.
#
# usage: x86_64-multiboot2.sh <image> <initramfs>
#
# The ISO is written to $PACKAGE_OUT (default: nyxos.iso).
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <image> <initramfs>" >&2
    exit 2
fi

image=$1
initramfs=$2
out=${PACKAGE_OUT:-nyxos.iso}

# distros ship GRUB tools as either grub-* or grub2-*
find_tool() {
    local t
    for t in "$@"; do
        if command -v "$t" >/dev/null; then
            echo "$t"
            return
        fi
    done
    echo "error: none of '$*' found" >&2
    return 1
}

grub_file=$(find_tool grub-file grub2-file)
grub_mkrescue=$(find_tool grub-mkrescue grub2-mkrescue)

if ! "$grub_file" --is-x86-multiboot2 "$image"; then
    echo "error: '$image' has no valid multiboot2 header" >&2
    exit 1
fi

isodir=$(mktemp -d)
trap 'rm -rf "$isodir"' EXIT

mkdir -p "$isodir/boot/grub"
cp "$image" "$isodir/boot/image"
cp "$initramfs" "$isodir/boot/initramfs"

# the prekernel looks for a module named "initramfs"
cat >"$isodir/boot/grub/grub.cfg" <<CFG
set timeout=0
set default=0
menuentry "nyxos" {
    multiboot2 /boot/image
    module2    /boot/initramfs "initramfs"
}
CFG

mkdir -p "$(dirname "$out")"
# xorriso is noisy, only show its output on failure
if ! log=$("$grub_mkrescue" -o "$out" "$isodir" 2>&1); then
    echo "$log" >&2
    echo "error: $grub_mkrescue failed" >&2
    exit 1
fi
