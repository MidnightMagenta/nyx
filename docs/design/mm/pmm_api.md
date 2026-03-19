## Physical memory manager API

Initialization:

- `void pm_init()`

Basic manipulation:

- `int pm_reserve_region(phys_addr_t addr, size_t size)`
- `int pm_free_region(phys_addr_t addr, size_t size)`

Allocation and freeing:

- `phys_addr_t pm_alloc_page()`
- `void pm_free_page(phys_addr_t page)`
- `phys_addr_t pm_alloc_pages(size_t count, phys_addr_t align, u32 flags)`
- `void pm_free_pages(phys_addr_t base, size_t count)`

All allocations are contiguous.

`pm_alloc_pages_ex()` flags:

| bit   | flag              | description                                                                                                                                               |
| ----- | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0     | PM_MEM_LOMEM      | Memory between 0 and 1 MiB                                                                                                                                |
| 1     | PM_MEM_NORMAL     | Memory between 1 MiB and 4 GiB                                                                                                                            |
| 2     | PM_MEM_HIMEM      | Memory between 4 GiB and the ISA limit                                                                                                                    |
| 3-8   | PM_MBZ            | Reserved for future use. Must be zero                                                                                                                     |
| 9     | PM_ZERO           | Return pre-zeroed memory                                                                                                                                  |
| 10    | PM_ALLOC_BOTTOMUP | Allocate starting at the lowest address first. By default (if this bit is cleared) the allocator will allocate starting from the highest address          |
| 11    | PM_ALLOC_RELAX    | If this bit is set, if memory from a requested pool is not available, the allocator may fall back to a different pool. Otherwise the allocation must fail |
| 12-31 | PM_MBZ            | Reserved for future use. Must be zero                                                                                                                     |

If a combination of flags `PM_MEM_LOMEM`, `PM_MEM_NORMAL`, or `PM_MEM_HIMEM` is used, the kernel will perform an allocation from the sum of these pools. For instance, if `PM_MEM_LOMEM` and `PM_MEM_HIMEM` is used, the kernel can allocate memory either between `0x0` and `0x100000` or `0x100000000` and `0xFFFFFFFFFFFFFFFF` (architectural limits may apply). If all `PM_MEM_*` fields are clear, the kernel will default to allocating from only the `PM_MEM_HIMEM` pool. I.e. bit 0, 1, and 2 being clear is equivalent to passing `PM_MEM_HIMEM`.

All remaining allocating functions will allocate from the pool of `PM_MEM_HIMEM`, unless that pool is unavailable or exhausted.

Additionally the following macros must be defined:

- `PM_MEM_ANY` is equal to `PM_MEM_LOMEM | PM_MEM_NORMAL | PM_MEM_HIMEM`.
- `PM_MEM_DMA16` is equal to `PM_MEM_LOMEM`
- `PM_MEM_DMA32` is equal to `PM_MEM_LOMEM | PM_MEM_NORMAL`
- `PM_MEM_PREFERRED` defined by the architecture as the preferred allocation range
- `PM_ALLOC_TOPDOWN` set to zero (`PM_ALLOC_BOTTOMUP` bit clear)

Values of `PM_ALIGN` lower than `PM_MIN_ALIGN` are equivalent to `PM_MIN_ALIGN`

Values of `PM_ALIGN` greater than `PM_MAX_ALIGN` cause the allocation to fail

All allocation functions which do not take a flags field allocate as if they had been given the flags of `PM_MEM_PREFERRED | PM_MEM_ALIGN(PM_MIN_ALIGN) | PM_ALLOC_TOPDOWN | PM_ALLOC_RELAX`

Stats:

- `bool pm_is_free(phys_addr_t page)`
- `bool pm_is_reserved(phys_addr_t page)`
- `size_t pm_getstat(u16 stat)`

`pm_getstat` stat values:

- PM_MIN_ALIGN
- PM_MAX_ALIGN
- PM_STAT_TOTAL
- PM_STAT_FREE
- PM_STAT_USED
- PM_STAT_RESERVED
- PM_STAT_FREE_LOMEM
- PM_STAT_USED_LOMEM
- PM_STAT_RESERVED_LOMEM
- PM_STAT_FREE_NORMAL
- PM_STAT_USED_NORMAL
- PM_STAT_RESERVED_NORMAL
- PM_STAT_FREE_HIMEM
- PM_STAT_USED_HIMEM
- PM_STAT_RESERVED_HIMEM
- PM_STAT_LAST_ERROR
