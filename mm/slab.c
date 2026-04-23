#include <asi/bitops.h>
#include <asi/page.h>

#include <mm/memory.h>
#include <mm/mm_types.h>
#include <mm/physmem.h>
#include <mm/slab.h>
#include <nyx/errno.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/string.h>
#include <nyx/types.h>

#define pr_fmt(fmt) "slab: " fmt "\n"

#define SLAB_MAX_SMALL_OBJECT (PAGE_SIZE / 8)
#define SLAB_WASTE_THRESHOLD  15 // percent

#define CFLAG_OFF_SLAB (1 << 0)

#define OFF_SLAB(cache) test_bit(CFLAG_OFF_SLAB, &(cache)->flags)

typedef u16 kmem_bufctl_t;

#define BUFCTL_END ((kmem_bufctl_t) (~0U))

struct kmem_slab_s {
    struct list_head list;
    void            *slab_mem;
    void            *slab_addr;
    u16              in_use;
    u16              free;
};

static_assert(sizeof(struct kmem_slab_s) < SLAB_MAX_SMALL_OBJECT, "kmem_slab_s must be a small object");

struct kmem_list_s {
    struct list_head slabs_full;
    struct list_head slabs_partial;
    struct list_head slabs_empty;

    size_t free_objects;
};

struct kmem_cache_s {
    struct list_head list;

    u64 flags;

    struct kmem_list_s slab_lists;

    unsigned int gfp_order;
    unsigned int gfp_flags;

    size_t obj_size;
    size_t obj_align;
    size_t nr_objs;
    void   (*ctor)(void *obj);
    void   (*dtor)(void *obj);

    struct kmem_cache_s *slab_cache;

    char name[32];
};

static_assert(sizeof(struct kmem_cache_s) < SLAB_MAX_SMALL_OBJECT, "kmem_cache_t must be a small object");

static struct kmem_cache_s cache_cache;
static LIST_HEAD(cache_chain);

static void *kmem_getpages(struct kmem_cache_s *cache, int local_flags) {
    struct page *page, *p;
    int          i;
    (void) local_flags;

    page = pm_alloc_pages(cache->gfp_flags, cache->gfp_order);
    if (!page) { return NULL; }

    i = (1 << cache->gfp_order);
    p = page;
    while (i--) { SetPageSlab(p++); }
    return page_address(page);
}

static void kmem_freepages(struct kmem_cache_s *cache, void *addr) {
    struct page *page, *p;
    int          i;

    page = virt_to_page(addr);

    p = page;
    i = (1 << cache->gfp_order);
    while (i--) { ClearPageSlab(p++); }

    __pm_free_pages(page, cache->gfp_order);
}

static struct kmem_slab_s *alloc_slabmgmt(struct kmem_cache_s *cache, void *objptr, int local_flags) {
    struct kmem_slab_s *slab;

    if (OFF_SLAB(cache)) {
        slab           = kmem_cache_alloc(cache->slab_cache, local_flags);
        slab->slab_mem = objptr;
    } else {
        slab           = objptr;
        slab->slab_mem = objptr + (sizeof(struct kmem_slab_s) + cache->nr_objs * sizeof(kmem_bufctl_t));
    }

    slab->in_use    = 0;
    slab->free      = 0;
    slab->slab_addr = objptr;

    return slab;
}

static void slab_map_pages(struct kmem_cache_s *cache, struct kmem_slab_s *slab, void *addr, int local_flags) {
    int          nr_pages;
    struct page *page;
    (void) local_flags;

    page     = virt_to_page(addr);
    nr_pages = (1 << cache->gfp_order);

    do {
        page->kmem_cache = cache;
        page->kmem_slab  = slab;
        page++;
    } while (nr_pages--);
}

static inline kmem_bufctl_t *slab_bufctl(struct kmem_slab_s *slab) {
    return (kmem_bufctl_t *) (slab + 1);
}

static inline void *slab_idx_to_obj(struct kmem_cache_s *cache, struct kmem_slab_s *slab, int i) {
    return slab->slab_mem + cache->obj_size * i;
}

static void cache_init_objs(struct kmem_cache_s *cache, struct kmem_slab_s *slab, int local_flags) {
    size_t i;
    (void) local_flags;

    for (i = 0; i < cache->nr_objs; i++) {
        void *objp = slab_idx_to_obj(cache, slab, i);
        if (cache->ctor) { cache->ctor(objp); }
        slab_bufctl(slab)[i] = i + 1;
    }
    slab_bufctl(slab)[i - 1] = BUFCTL_END;
}

static int cache_grow(struct kmem_cache_s *cache, int local_flags) {
    void               *objptr;
    struct kmem_slab_s *slab;

    objptr = kmem_getpages(cache, local_flags);
    if (!objptr) { goto failed0; }

    slab = alloc_slabmgmt(cache, objptr, local_flags);
    if (!slab) { goto failed1; }

    slab_map_pages(cache, slab, objptr, local_flags);
    cache_init_objs(cache, slab, local_flags);

    list_add_tail(&slab->list, &cache->slab_lists.slabs_empty);
    cache->slab_lists.free_objects += cache->nr_objs;
    return 1;

failed1:
    kmem_freepages(cache, objptr);
failed0:
    return 0;
}

static void cache_destroy_objs(struct kmem_cache_s *cache, struct kmem_slab_s *slab) {
    if (!cache->dtor) { return; }
    for (size_t i = 0; i < cache->nr_objs; i++) {
        void *objp = slab_idx_to_obj(cache, slab, i);
        cache->dtor(objp);
    }
}

int __init kmem_cache_init() {
    cache_cache.list  = (struct list_head) LIST_HEAD_INIT(cache_cache.list);
    cache_cache.flags = 0;

    cache_cache.slab_lists.slabs_empty   = (struct list_head) LIST_HEAD_INIT(cache_cache.slab_lists.slabs_empty);
    cache_cache.slab_lists.slabs_partial = (struct list_head) LIST_HEAD_INIT(cache_cache.slab_lists.slabs_partial);
    cache_cache.slab_lists.slabs_full    = (struct list_head) LIST_HEAD_INIT(cache_cache.slab_lists.slabs_full);
    cache_cache.slab_lists.free_objects  = 0;

    cache_cache.gfp_order = 0;
    cache_cache.gfp_flags = GFP_ATOMIC;

    cache_cache.obj_size  = sizeof(struct kmem_cache_s);
    cache_cache.obj_align = _Alignof(struct kmem_cache_s);
    cache_cache.nr_objs =
            (PAGE_SIZE - sizeof(struct kmem_slab_s)) / (sizeof(struct kmem_cache_s) + sizeof(kmem_bufctl_t));
    cache_cache.ctor = NULL;
    cache_cache.dtor = NULL;

    cache_cache.slab_cache = NULL;

    strcpy(cache_cache.name, "kmem_cache");

    list_add_tail(&cache_cache.list, &cache_chain);

    return 0;
}

static void calculate_slab_size(struct kmem_cache_s *cache) {
    size_t order, nr_objs;

    if (cache->obj_size > SLAB_MAX_SMALL_OBJECT) { cache->flags |= CFLAG_OFF_SLAB; }

    for (order = 0; order < MAX_ORDER; order++) {
        size_t slab_size = (1 << order) << PAGE_SHIFT;
        if (OFF_SLAB(cache)) {
            nr_objs = slab_size / ALIGN_UP(cache->obj_size, cache->obj_align);
        } else {
            nr_objs = (slab_size - sizeof(struct kmem_slab_s)) /
                      (ALIGN_UP(cache->obj_size, cache->obj_align) + sizeof(kmem_bufctl_t));
        }
        size_t waste = slab_size - nr_objs * cache->obj_size;
        if (waste * 100 / slab_size < SLAB_WASTE_THRESHOLD) { break; }
    }

    cache->gfp_order = order;
    cache->nr_objs   = nr_objs;
}

kmem_cache_t *kmem_create_cache(const char *name,
                                size_t      size,
                                size_t      align,
                                void        (*ctor)(void *),
                                void        (*dtor)(void *),
                                int         gfp_flags) {
    struct kmem_cache_s *cache;

    if (!name || size == 0) { return NULL; }
    if (align == 0) { align = 1; }

    cache = kmem_cache_alloc(&cache_cache, 0);

    cache->list = (struct list_head) LIST_HEAD_INIT(cache->list);

    cache->slab_lists.slabs_empty   = (struct list_head) LIST_HEAD_INIT(cache->slab_lists.slabs_empty);
    cache->slab_lists.slabs_partial = (struct list_head) LIST_HEAD_INIT(cache->slab_lists.slabs_partial);
    cache->slab_lists.slabs_full    = (struct list_head) LIST_HEAD_INIT(cache->slab_lists.slabs_full);
    cache->slab_lists.free_objects  = 0;

    cache->obj_size  = size;
    cache->obj_align = align;
    cache->nr_objs   = 0;
    cache->ctor      = ctor;
    cache->dtor      = dtor;

    strcpy(cache->name, name);

    calculate_slab_size(cache);

    if (OFF_SLAB(cache)) {
        cache->slab_cache = kmem_create_cache("kmem_slab",
                                              sizeof(struct kmem_slab_s),
                                              _Alignof(struct kmem_slab_s),
                                              NULL,
                                              NULL,
                                              gfp_flags);
    } else {
        cache->slab_cache = NULL;
    }

    list_add_tail(&cache->list, &cache_chain);

    return cache;
}

void *kmem_cache_alloc(kmem_cache_t *cache, int flags) {
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
    // if __DEBUG
    //      run destructor
    //      memset to 0xAF
}

int kmem_cache_destroy(kmem_cache_t *cache) {
    int res;

    if (!cache) { return -EINVAL; }
    if (!list_is_empty(&cache->slab_lists.slabs_partial) || !list_is_empty(&cache->slab_lists.slabs_full)) {
        return -EINVAL;
    }

    while (!list_is_empty(&cache->slab_lists.slabs_empty)) {
        struct kmem_slab_s *slab = list_entry(cache->slab_lists.slabs_empty.next, struct kmem_slab_s, list);
        void               *addr = slab->slab_addr;
        if (cache->dtor) { cache_destroy_objs(cache, slab); }

        list_del(&slab->list);
        kmem_freepages(cache, addr);
        if (OFF_SLAB(cache)) { kmem_cache_free(cache->slab_cache, slab); }
    }

    if (OFF_SLAB(cache)) {
        if ((res = kmem_cache_destroy(cache->slab_cache))) { return res; };
    }

    list_del(&cache->list);
    kmem_cache_free(&cache_cache, cache);

    return 0;
}
