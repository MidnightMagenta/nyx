#include <asi/memory.h>
#include <asi/page.h>
#include <mm/mm_types.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <nyx/align.h>
#include <nyx/errno.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/panic.h>
#include <nyx/printk.h>
#include <nyx/string.h>

#define pr_fmt(fmt) "slab: " fmt "\n"

#define SLAB_MAX_SMALL_OBJECT (PAGE_SIZE / 8)

#define CFLAG_OFF_SLAB (1 << 0)

typedef u16 kmem_bufctl_t;

struct kmem_slab_s {
    struct list_head list;
    void            *slab_mem;
    u16              in_use;
    u16              free;
};

struct kmem_list_s {
    struct list_head slabs_full;
    struct list_head slabs_partial;
    struct list_head slabs_empty;

    size_t free_objects;
    size_t used_objects;
};

struct kmem_cache_s {
    struct list_head list;

    u64 flags;

    struct kmem_list_s slab_lists;

    u64 pm_flags;
    u64 pm_order;

    size_t obj_size;
    size_t obj_align;
    void   (*ctor)(void *obj);
    void   (*dtor)(void *obj);

    char name[32];
};

static struct kmem_cache_s cache_cache = {
        LIST_HEAD_INIT(cache_cache.list),
        0,
        {
                .slabs_full    = LIST_HEAD_INIT(cache_cache.slab_lists.slabs_full),
                .slabs_partial = LIST_HEAD_INIT(cache_cache.slab_lists.slabs_partial),
                .slabs_empty   = LIST_HEAD_INIT(cache_cache.slab_lists.slabs_empty),

                0,
                0,
        },
        PMF_NOSLEEP,
        1,
        sizeof(kmem_cache_t),
        8,
        NULL,
        NULL,
        "cache_cache",
};

static LIST_HEAD(cache_chain);

int __init kmem_cache_init() {
    list_add(&cache_cache.list, &cache_chain);
    return 0;
}

kmem_cache_t *kmem_create_cache(const char *name,
                                size_t      size,
                                size_t      align,
                                void        (*ctor)(void *),
                                void        (*dtor)(void *)) {}

static int kmem_cache_grow(kmem_cache_t *cache, u64 flags) {
    return 0;
}

void *kmem_cache_alloc(kmem_cache_t *cache, u64 flags) {
    // if !cache.partal && !cache.empty
    //      allocate a new slab
    //      if cache.flags & CFLAG_OFF_SLAB
    //          if hashmap_should_grow() && !hashmap.flags & HASHFLAG_IN_FLIGHT
    //              grow_hashmap()
    //          allocate kmem_slab and kmem_bufctl's for the new slab
    //          link the kmem_slab and kmem_bufctl's
    //      else
    //          allocate a new slab
    //          place kmem_cache at the end
    //          place kmem_bufctl's appropriately inside the slab
    // if !cache.partial
    //      carve a slab from empty list
    // else
    //      allocate from the partial slabs first
    //
    // yoink a new slot from the chosen slab
    // if cache.ctor
    //     construct the new slab
    //
    // return the pointer to the fresh slot
}

void *kmem_cache_free(kmem_cache_t *cache, void *obj) {
    // if cache.flags & CFLAG_OFF_SLAB
    //     kmem_bufctl = hashmap[obj]
    //     kmem_bufctl.next = kmem_bufctl.slab.first
    //     kmem_bufctl.slab.first = kmem_bufctl
    // else
    //     kmem_bufctl = obj - sizeof(void*)
    //     kmem_slab = (kmem_slab)(ALIGN_UP(obj, PAGE_SIZE) - sizeof(kmem_slab))
    //     kmem_bufctl = kmem_slab.first
    //     kmem_slab.first = kmem_bufctl
    //
    // if CONFIG_DEBUG
    //      run destructor
    //      memset to 0xAF
}

void kmem_cache_destroy(kmem_cache_t *cache) {}
