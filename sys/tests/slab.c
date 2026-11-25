#include <sys/errno.h>
#include <sys/mm/address.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/slab.h>
#include <sys/string.h>
#include <sys/testing.h>

#include <asi/page.h>

KERNEL_TEST(slab_sanity) {
    kmem_cache_t *cache;
    void         *test_alloc;

    cache = kmem_create_cache("slab_sanity", 8, 8, NULL, NULL, 0);
    ASSERT_TRUE(cache);

    test_alloc = kmem_cache_alloc(cache, 0);
    ASSERT_TRUE(test_alloc);

    *((char *) test_alloc) = 0;

    kmem_cache_free(cache, test_alloc);
    ASSERT_FALSE(kmem_cache_destroy(cache));
}

KERNEL_TEST(slab_large_sanity) {
    kmem_cache_t *cache;
    void         *test_alloc;

    cache = kmem_create_cache("slab_sanity", PAGE_SIZE / 2, PAGE_SIZE / 2, NULL, NULL, 0);
    ASSERT_TRUE(cache);

    test_alloc = kmem_cache_alloc(cache, 0);
    ASSERT_TRUE(test_alloc);

    *((char *) test_alloc) = 0;

    kmem_cache_free(cache, test_alloc);
    ASSERT_FALSE(kmem_cache_destroy(cache));
}

KERNEL_TEST(slab_invalid_args) {
    EXPECT_NULL(kmem_create_cache(NULL, 8, 8, NULL, NULL, 0));
    EXPECT_NULL(kmem_create_cache("slab_zero", 0, 8, NULL, NULL, 0));
    EXPECT_NULL(kmem_cache_alloc(NULL, 0));
    EXPECT_EQ(kmem_cache_destroy(NULL), -EINVAL);

    kmem_cache_free(NULL, NULL);
}

static void slab_stress(struct test_state *state, size_t size, size_t align, size_t count) {
    static void  *objs[512];
    kmem_cache_t *cache;
    size_t        n, misaligned = 0;

    if (count > ARRAY_SIZE(objs)) { count = ARRAY_SIZE(objs); }

    cache = kmem_create_cache("slab_stress", size, align, NULL, NULL, 0);
    ASSERT_NOT_NULL(cache);

    for (n = 0; n < count; n++) {
        objs[n] = kmem_cache_alloc(cache, 0);
        if (!objs[n]) {
            ADD_FAILURE("size %ld: allocation %ld failed", (long) size, (long) n);
            break;
        }
        if ((uintptr_t) objs[n] & (align - 1)) {
            if (!misaligned) {
                ADD_FAILURE("size %ld: object %p not aligned to %ld", (long) size, objs[n], (long) align);
            }
            misaligned++;
        }
        memset(objs[n], (int) (n & 0xFF), size);
    }
    EXPECT_EQ(misaligned, 0);

    for (size_t i = 0; i < n && !TEST_HAS_FAILED(); i++) {
        const unsigned char *p = objs[i];
        for (size_t j = 0; j < size; j++) {
            if (p[j] != (unsigned char) (i & 0xFF)) {
                ADD_FAILURE("size %ld: object %ld was overwritten at offset %ld", (long) size, (long) i, (long) j);
                break;
            }
        }
    }

    for (size_t i = 0; i < n; i += 2) { kmem_cache_free(cache, objs[i]); }
    for (size_t i = 1; i < n; i += 2) { kmem_cache_free(cache, objs[i]); }

    EXPECT_OK(kmem_cache_destroy(cache));
}

KERNEL_TEST(slab_small_objects) {
    ASSERT_NO_FAILURE(slab_stress(state, 1, 1, 300));
    ASSERT_NO_FAILURE(slab_stress(state, 8, 8, 512));
    ASSERT_NO_FAILURE(slab_stress(state, 24, 8, 400));
    ASSERT_NO_FAILURE(slab_stress(state, 100, 4, 300));
    ASSERT_NO_FAILURE(slab_stress(state, 128, 64, 300));
}

KERNEL_TEST(slab_off_slab_objects) {
    ASSERT_NO_FAILURE(slab_stress(state, PAGE_SIZE / 8 + 1, 8, 64));
    ASSERT_NO_FAILURE(slab_stress(state, 1000, 8, 64));
    ASSERT_NO_FAILURE(slab_stress(state, PAGE_SIZE, PAGE_SIZE, 32));
    ASSERT_NO_FAILURE(slab_stress(state, 3 * PAGE_SIZE, PAGE_SIZE, 16));
}

KERNEL_TEST(slab_reuse) {
    kmem_cache_t *cache;
    void         *a, *b;

    cache = kmem_create_cache("slab_reuse", 32, 8, NULL, NULL, 0);
    ASSERT_NOT_NULL(cache);

    a = kmem_cache_alloc(cache, 0);
    ASSERT_NOT_NULL(a);
    kmem_cache_free(cache, a);
    b = kmem_cache_alloc(cache, 0);
    EXPECT_EQ(a, b);

    kmem_cache_free(cache, b);
    EXPECT_OK(kmem_cache_destroy(cache));
}

KERNEL_TEST(slab_destroy_busy) {
    kmem_cache_t *cache;
    void         *obj;

    cache = kmem_create_cache("slab_busy", 64, 8, NULL, NULL, 0);
    ASSERT_NOT_NULL(cache);
    obj = kmem_cache_alloc(cache, 0);
    ASSERT_NOT_NULL(obj);

    EXPECT_EQ(kmem_cache_destroy(cache), -EINVAL);

    kmem_cache_free(cache, obj);
    EXPECT_OK(kmem_cache_destroy(cache));
}

KERNEL_TEST(slab_free_wrong_cache) {
    kmem_cache_t *c1, *c2;
    void         *obj;

    c1 = kmem_create_cache("slab_c1", 64, 8, NULL, NULL, 0);
    c2 = kmem_create_cache("slab_c2", 64, 8, NULL, NULL, 0);
    ASSERT_NOT_NULL(c1);
    ASSERT_NOT_NULL(c2);

    obj = kmem_cache_alloc(c1, 0);
    ASSERT_NOT_NULL(obj);

    kmem_cache_free(c2, obj);
    EXPECT_EQ(kmem_cache_destroy(c1), -EINVAL);

    kmem_cache_free(c1, obj);
    EXPECT_OK(kmem_cache_destroy(c1));
    EXPECT_OK(kmem_cache_destroy(c2));
}

static int ctor_calls, dtor_calls;

struct ctor_obj {
    u32 magic;
    u32 pad[7];
};

static void test_ctor(void *p) {
    ((struct ctor_obj *) p)->magic = 0xC0FFEE;
    ctor_calls++;
}

static void test_dtor(void *p) {
    (void) p;
    dtor_calls++;
}

KERNEL_TEST(slab_ctor_dtor) {
    kmem_cache_t    *cache;
    struct ctor_obj *obj;

    ctor_calls = dtor_calls = 0;

    cache = kmem_create_cache("slab_ctor", sizeof(struct ctor_obj), 8, test_ctor, test_dtor, 0);
    ASSERT_NOT_NULL(cache);

    obj = kmem_cache_alloc(cache, 0);
    ASSERT_NOT_NULL(obj);
    EXPECT_GT(ctor_calls, 0);
    EXPECT_EQ(obj->magic, 0xC0FFEE);
    EXPECT_EQ(dtor_calls, 0);

    kmem_cache_free(cache, obj);
    EXPECT_OK(kmem_cache_destroy(cache));

    EXPECT_EQ(dtor_calls, ctor_calls);
}

KERNEL_TEST(kmalloc_sizes) {
    static const unsigned long sizes[] = {1, 7, 8, 9, 16, 33, 100, 128, 255, 512, 1000, 2048, 4095, 4096};
    void                      *ptrs[ARRAY_SIZE(sizes)];

    for (size_t i = 0; i < ARRAY_SIZE(sizes); i++) {
        unsigned long sz    = sizes[i];
        unsigned long align = 8;

        while (align < sz && align < PAGE_SIZE) { align <<= 1; }

        ptrs[i] = kmalloc(sz, 0);
        if (!ptrs[i]) {
            ADD_FAILURE("kmalloc(%ld) failed", (long) sz);
            continue;
        }
        if ((uintptr_t) ptrs[i] & (align - 1)) {
            ADD_FAILURE("kmalloc(%ld) returned %p, not aligned to %ld", (long) sz, ptrs[i], (long) align);
        }
        memset(ptrs[i], (int) i + 1, sz);
    }

    for (size_t i = 0; i < ARRAY_SIZE(sizes); i++) {
        const unsigned char *p = ptrs[i];

        if (!p) { continue; }
        for (size_t j = 0; j < sizes[i]; j++) {
            if (p[j] != (unsigned char) (i + 1)) {
                ADD_FAILURE("kmalloc(%ld) block overwritten at offset %ld", (long) sizes[i], (long) j);
                break;
            }
        }
        kfree(ptrs[i]);
    }
}

KERNEL_TEST(kmalloc_zero) {
    EXPECT_NULL(kmalloc(0, 0));
}

KERNEL_TEST(kmalloc_large) {
    static const unsigned long sizes[] = {4097, 8192, 3 * PAGE_SIZE, 64 * PAGE_SIZE};

    for (size_t i = 0; i < ARRAY_SIZE(sizes); i++) {
        unsigned char *p = kmalloc(sizes[i], 0);
        struct page   *pg;
        zone_t        *zone;
        size_t         free_after_alloc;

        if (!p) {
            ADD_FAILURE("kmalloc(%ld) failed", (long) sizes[i]);
            continue;
        }

        EXPECT_EQ((uintptr_t) p & (PAGE_SIZE - 1), 0);
        pg = virt_to_page(p);
        EXPECT_TRUE(PageKmalloc(pg));
        EXPECT_GE((unsigned long) PAGE_SIZE << pg->pg_head_order, sizes[i]);

        p[0]            = 1;
        p[sizes[i] - 1] = 2;

        zone             = &pgdata->zones[pg->pg_zone_id];
        free_after_alloc = zone->free_pages;
        kfree(p);
        EXPECT_EQ(zone->free_pages, free_after_alloc + (1ul << pg->pg_head_order));
        EXPECT_FALSE(PageKmalloc(pg));
    }
}

KERNEL_TEST(kmalloc_reuse) {
    void *a = kmalloc(48, 0);
    void *b;

    ASSERT_NOT_NULL(a);
    kfree(a);
    b = kmalloc(60, 0);
    EXPECT_EQ(a, b);
    kfree(b);
}

KERNEL_TEST(slab_shrink_then_alloc) {
    static void  *objs[64];
    kmem_cache_t *cache;

    cache = kmem_create_cache("slab_shrink", 1000, 8, NULL, NULL, 0);
    ASSERT_NOT_NULL(cache);

    for (int round = 0; round < 3; round++) {
        for (size_t i = 0; i < ARRAY_SIZE(objs); i++) {
            objs[i] = kmem_cache_alloc(cache, 0);
            ASSERT_NOT_NULL(objs[i]);
        }
        for (size_t i = 0; i < ARRAY_SIZE(objs); i++) { kmem_cache_free(cache, objs[i]); }
    }

    EXPECT_OK(kmem_cache_destroy(cache));
}
