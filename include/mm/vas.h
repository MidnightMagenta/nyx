#ifndef _MM_MM_H
#define _MM_MM_H

#include <mm/mm_types.h>
#include <nyx/atomic.h>

static inline void vas_get(struct vas_struct *vas) {
    atomic_inc(&vas->user_count);
}

void vas_put(struct vas_struct *vas);

static inline void vas_grab(struct vas_struct *vas) {
    atomic_inc(&vas->refcount);
}

void vas_drop(struct vas_struct *vas);

struct mm_struct *mm_alloc();
void              mm_map(struct mm_struct *mm, virt_addr_t vaddr, size_t size, u32 flags, int gfp_flags);
void mm_map_copy(struct mm_struct *mm, virt_addr_t vaddr, const void *data, size_t size, u32 flags, int gfp_flags);
void mm_free(struct mm_struct *mm);

#endif
