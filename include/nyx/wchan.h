#ifndef _NYX_WCHAN_H
#define _NYX_WCHAN_H

#include <nyx/list.h>
#include <nyx/stddef.h>

#define NR_SLEEP_BUCKETS 128

extern struct list_head waittab[NR_SLEEP_BUCKETS];

static inline size_t wchan_hash(const void *wchan, size_t table_size) {
    uintptr_t key = (uintptr_t) wchan;
    return (key * 2654435761UL) >> (32 - __builtin_ctz(table_size));
}

#define WAITID(ident) wchan_hash((ident), NR_SLEEP_BUCKETS)

#endif
