#include <mm/slab.h>
#include <nyx/testing.h>

#include <asi/page.h>

KERNEL_TEST(slab_sanity) {
    kmem_cache_t *cache;
    void         *test_alloc;

    cache = kmem_create_cache("slab_sanity", 8, 8, NULL, NULL, 0);
    EXPECT_TRUE(cache);

    test_alloc = kmem_cache_alloc(cache, 0);
    EXPECT_TRUE(test_alloc);

    kmem_cache_free(cache, test_alloc);
    EXPECT_FALSE(kmem_cache_destroy(cache));
}

KERNEL_TEST(slab_large_sanity) {
    kmem_cache_t *cache;
    void         *test_alloc;

    cache = kmem_create_cache("slab_sanity", PAGE_SIZE / 2, PAGE_SIZE / 2, NULL, NULL, 0);
    EXPECT_TRUE(cache);

    test_alloc = kmem_cache_alloc(cache, 0);
    EXPECT_TRUE(test_alloc);

    kmem_cache_free(cache, test_alloc);
    EXPECT_FALSE(kmem_cache_destroy(cache));
}
