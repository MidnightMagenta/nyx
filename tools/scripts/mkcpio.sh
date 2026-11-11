#!/usr/bin/env bash
# Build a newc CPIO archive (e.g. the initramfs).
#
# usage: mkcpio.sh <out> <staging-dir> [<src>:<dest>]...
#
# <staging-dir> is wiped and repopulated with every <src> copied to <dest>
# (an absolute path inside the archive), then archived into <out>.
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "usage: $0 <out> <staging-dir> [<src>:<dest>]..." >&2
    exit 2
fi

out=$1
root=$2
shift 2

rm -rf "$root"
mkdir -p "$root/dev"

for mapping in "$@"; do
    src=${mapping%%:*}
    dest=${mapping#*:}
    if [[ $dest != /* ]]; then
        echo "error: archive path '$dest' must be absolute" >&2
        exit 1
    fi
    mkdir -p "$root$(dirname "$dest")"
    cp "$src" "$root$dest"
done

# sorted and with fixed ownership, so the archive is reproducible
(cd "$root" && find . | LC_ALL=C sort | cpio -o -H newc --owner=0:0 --reproducible --quiet) >"$out.tmp"
mv "$out.tmp" "$out"
