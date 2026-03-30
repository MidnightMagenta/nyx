#ifndef _MM_SLAB_H
#define _MM_SLAB_H

#include <nyx/stddef.h>
#include <nyx/types.h>

typedef struct kmem_cache_s kmem_cache_t;

kmem_cache_t *kmem_create_cache(const char *name,
                                size_t      size,
                                size_t      align,
                                void        (*ctor)(void *),
                                void        (*dtor)(void *));
void         *kmem_cache_alloc(kmem_cache_t *cache, u64 flags);
void         *kmem_cache_free(kmem_cache_t *cache, void *obj);
void          kmem_cache_destroy(kmem_cache_t *cache);

#endif
