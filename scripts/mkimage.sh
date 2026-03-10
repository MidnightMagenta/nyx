#! /bin/sh
set -euo pipefail

grub-file --is-x86-multiboot2 image
if [[ $? -eq 1 ]]; then
    echo -e "Invalid multiboot2 header"
    exit
fi

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp image isodir/boot/image
cat >isodir/boot/grub/grub.cfg <<EOF
set timeout=0
set default=0
menuentry "nyxos" {
    multiboot2 /boot/image
}
EOF

grub-mkrescue -o nyxos.iso isodir
