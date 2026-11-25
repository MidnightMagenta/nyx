#include <sys/linkage.h>
#include <sys/vfs.h>

#include "internal.h"

extern void ramfs_init();

void __init init_vfs() {
    dcache_init();
    inode_init();
    file_init();
    filedesc_init();
}

void __init init_filesystems() {
    ramfs_init();
}
