#ifndef _ASI_X86_WCHAN_H
#define _ASI_X86_WCHAN_H

#include <nyx/list.h>
#include <nyx/stddef.h>

#define NR_SLEEP_BUCKETS 128

extern struct list_head waittab[NR_SLEEP_BUCKETS];

static inline size_t wchan_hash(const volatile void *wchan, size_t table_size) {
    u64 key = (uintptr_t) wchan;
    u64 h   = key * 11400714819323198485ULL;
    return h >> (64 - __builtin_ctz((u32) table_size));
}

#define WAITID(ident) wchan_hash((ident), NR_SLEEP_BUCKETS)

#endif
