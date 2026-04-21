#include <mm/physmem.h>
#include <nyx/testing.h>

KERNEL_TEST(physmem_round_trip) {
    struct page *first_page, *second_page;
    first_page = pm_alloc_pages(0, 0);
    __pm_free_pages(first_page, 0);
    second_page = pm_alloc_pages(0, 0);
    EXPECT_EQ(first_page, second_page);

    __pm_free_pages(second_page, 0);
}

// TODO: write physmem allocator tests
