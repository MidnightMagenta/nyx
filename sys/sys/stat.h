#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/uapi/nyx/stat.h>

#define S_IFSOCK __KERNEL_S_IFSOCK
#define S_IFLNK  __KERNEL_S_IFLNK
#define S_IFREG  __KERNEL_S_IFREG
#define S_IFBLK  __KERNEL_S_IFBLK
#define S_IFDIR  __KERNEL_S_IFDIR
#define S_IFCHR  __KERNEL_S_IFCHR
#define S_IFIFO  __KERNEL_S_IFIFO
#define S_IFMT   __KERNEL_S_IFMT

#define S_IRWXU __KERNEL_S_IRWXU
#define S_IRUSR __KERNEL_S_IRUSR
#define S_IWUSR __KERNEL_S_IWUSR
#define S_IXUSR __KERNEL_S_IXUSR
#define S_IRWXG __KERNEL_S_IRWXG
#define S_IRGRP __KERNEL_S_IRGRP
#define S_IWGRP __KERNEL_S_IWGRP
#define S_IXGRP __KERNEL_S_IXGRP
#define S_IRWXO __KERNEL_S_IRWXO
#define S_IROTH __KERNEL_S_IROTH
#define S_IWOTH __KERNEL_S_IWOTH
#define S_IXOTH __KERNEL_S_IXOTH
#define S_ISUID __KERNEL_S_ISUID
#define S_ISGID __KERNEL_S_ISGID
#define S_ISVTX __KERNEL_S_ISVTX

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#endif
