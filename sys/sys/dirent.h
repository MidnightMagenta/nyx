#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#include <sys/stat.h>
#include <sys/uapi/nyx/dirent.h>

#define DT_UNKNOWN __KERNEL_DT_UNKNOWN
#define DT_FIFO    __KERNEL_DT_FIFO
#define DT_CHR     __KERNEL_DT_CHR
#define DT_DIR     __KERNEL_DT_DIR
#define DT_BLK     __KERNEL_DT_BLK
#define DT_REG     __KERNEL_DT_REG
#define DT_LNK     __KERNEL_DT_LNK
#define DT_SOCK    __KERNEL_DT_SOCK

#define IFTODT(mode) (((mode) & S_IFMT) >> 12)
#define DTTOIF(type) ((type) << 12)

#endif
