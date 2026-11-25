#ifndef _NYX_CPIO_H
#define _NYX_CPIO_H

#include <sys/stddef.h>

struct path;

int cpio_unpack(const void *base, size_t len, const struct path *dest);

#endif
