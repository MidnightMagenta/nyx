#include "elf.h"
#include "multiboot2.h"
#include "printb.h"

#define hcf()                                                                                                          \
    while (1) { __asm__ volatile("hlt"); }

#define PAGE_SIZE 0x1000

#define ALIGN_UP(v, a)   ((v + (a - 1)) & (~(a - 1)))
#define ALIGN_DOWN(v, a) (v & ~(a - 1))

#define UNIT_B   1
#define UNIT_KiB 1024
#define UNIT_MiB 1024 * UNIT_KiB
#define UNIT_GiB 1024 * UNIT_MiB
#define UNIT_TiB 1023 * UNIT_GiB

#undef NULL
#define NULL ((void *) 0)

extern char __kernel_blob_start;
extern char __kernel_blob_end;
extern char __kernel_blob_size;

void         *kernel_blob_start = &__kernel_blob_start;
void         *kernel_blob_end   = &__kernel_blob_end;
unsigned long kernrel_blob_size = (unsigned long) &__kernel_blob_size;

int boot_memcmp(const void *a, const void *b, unsigned long n) {
    const unsigned char *aptr = (const unsigned char *) a;
    const unsigned char *bptr = (const unsigned char *) b;
    while (n--) {
        if (*aptr != *bptr) { return *aptr - *bptr; }
        aptr++;
        bptr++;
    }
    return 0;
}

void *boot_memset(void *p, int v, unsigned long num) {
    unsigned char *strPtr = (unsigned char *) p;
    while (num--) { *strPtr++ = (unsigned char) v; }
    return p;
}

void *boot_memcpy(void *restrict dst, void *restrict src, unsigned long num) {
    unsigned char       *dstPtr = (unsigned char *) dst;
    const unsigned char *srcPtr = (const unsigned char *) src;
    while (num--) { *dstPtr++ = *srcPtr++; }
    return dst;
}

#define BOOT_MAX_RESERVED_REGIONS 128

struct memregion {
    unsigned long base; // inclusive
    unsigned long end;  // exclusive
};

static struct memregion reserved_regions[BOOT_MAX_RESERVED_REGIONS];
static unsigned int     reserved_region_count = 0;

static void reserve_region(unsigned long base, unsigned long end) {
    if (reserved_region_count >= BOOT_MAX_RESERVED_REGIONS) { hcf(); }
    reserved_regions[reserved_region_count++] = (struct memregion) {base, end};
}

extern char __image_start;
extern char __image_end;

static void *image_start = &__image_start;
static void *image_end   = &__image_end;

static void reserve_boot_regions(unsigned long bi) {
    unsigned int    bi_size = *(unsigned int *) bi;
    struct mb2_tag *bi_tag  = (struct mb2_tag *) (bi + 8);
    struct mb2_tag *bi_end  = (struct mb2_tag *) (bi + bi_size);

    // reserve kernel image region
    reserve_region(ALIGN_DOWN((unsigned long) image_start, PAGE_SIZE), ALIGN_UP((unsigned long) image_end, PAGE_SIZE));

    // reserve boot info region
    reserve_region(ALIGN_DOWN(bi, PAGE_SIZE), ALIGN_UP(bi + bi_size, PAGE_SIZE));


    while (bi_tag <= bi_end && bi_tag->type != MB2_TAG_END) {
        if (bi_tag->type == MB2_TAG_MODULE) {
            struct mb2_tag_module *bi_mod = (struct mb2_tag_module *) bi_tag;
            reserve_region((unsigned long) bi_mod->mod_start, (unsigned long) bi_mod->mod_end);
        }
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }
}

static struct memregion subtract_region(struct memregion a, struct memregion b) {
    struct memregion res = {0};

    // check both regions are sane
    if (a.end <= a.base || b.end <= b.base) { return (struct memregion) {0}; }

    // case 0: regions fully overlap
    if (a.base == b.base && a.end == b.end) { return (struct memregion) {0}; }

    // case 1: regions don't overlap
    if (b.end <= a.base || b.base >= a.end) {
        res = a;
        return res;
    }

    // case 2: region b overlaps region a at the start
    if (b.end > a.base && b.base <= a.base) {
        res.base = a.base + (b.end - a.base);
        res.end  = a.end;
        return res;
    }

    // case 3: region b overlaps region a at the end
    if (b.base < a.end && b.end >= a.end) {
        res.base = a.base;
        res.end  = a.end - (a.end - b.base);
        return res;
    }

    // case 4: region b is fully inside region a (return the larger of the two non overlapping regions)
    if (b.base > a.base && b.end < a.end) {
        struct memregion lower = {a.base, b.base};
        struct memregion upper = {b.end, a.end};
        if (lower.end - lower.base > upper.end - upper.base) {
            return lower;
        } else {
            return upper;
        }
    }

    return (struct memregion) {0};
}

static const struct mb2_tag_mmap *get_mb2_mmap(unsigned long bi) {
    unsigned int    bi_size = *(unsigned int *) bi;
    struct mb2_tag *bi_tag  = (struct mb2_tag *) (bi + 8);
    struct mb2_tag *bi_end  = (struct mb2_tag *) (bi + bi_size);

    while (bi_tag <= bi_end && bi_tag->type != MB2_TAG_END) {
        if (bi_tag->type == MB2_TAG_MMAP) { return (struct mb2_tag_mmap *) bi_tag; }
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }

    return NULL;
}

struct memregion find_region(const struct mb2_tag_mmap *mmap, unsigned long reg_size, unsigned long align) {
    if (!mmap) {
        pr_error(pr_fmt("mmap == NULL"));
        hcf()
    }

    const struct mb2_tag_mmap *mmap_tag = mmap;
    if (mmap_tag->entry_version != 0) {
        pr_error(pr_fmt("Unknown mmap entry version %d"), mmap_tag->entry_version);
        hcf();
    }

    for (unsigned char *p = (unsigned char *) mmap_tag->entries; p < (unsigned char *) mmap_tag + mmap_tag->size;
         p += mmap_tag->entry_size) {
        struct mb2_mmap_entry *mmap_entry = (struct mb2_mmap_entry *) p;
        if (mmap_entry->type != MB2_MMAP_AVAILABLE) { continue; }

        struct memregion region = {mmap_entry->addr, mmap_entry->addr + mmap_entry->len};
        if (region.end - region.base < reg_size) { continue; }

        // region is larger than 4M
        for (unsigned int i = 0; i < reserved_region_count; ++i) {
            region = subtract_region(region, reserved_regions[i]);
        }

        region.base = ALIGN_UP(region.base, align);
        if (region.end < region.base || region.end - region.base < reg_size) { continue; }

        region.end = region.base + reg_size;
        reserve_region(region.base, region.end);
        return region;
    }

    return (struct memregion) {0};
}

static struct memregion bump_heap = {0, 0};

static void bump_heap_init(const struct mb2_tag_mmap *mmap) {
    bump_heap = find_region(mmap, 4 * UNIT_MiB, 0x1000);
    if (bump_heap.base == 0 && bump_heap.end == 0) {
        pr_error(pr_fmt("Could not initialize bump heap"));
        hcf();
    }
}

void *boot_alloc(unsigned long size) {
    if (bump_heap.base == 0 && bump_heap.end == 0) { return NULL; }
    if (size == 0 || bump_heap.base + size >= bump_heap.end) { return NULL; }
    void *allocation = (void *) bump_heap.base;
    bump_heap.base += size;
    return allocation;
}

void *boot_alloc_page() {
    if (bump_heap.base == 0 && bump_heap.end == 0) { return NULL; }
    if (ALIGN_UP(bump_heap.base, 0x1000) + 0x1000 > bump_heap.end) { return NULL; }
    void *allocation = (void *) ALIGN_UP(bump_heap.base, 0x1000);
    bump_heap.base += 0x1000;
    return allocation;
}

const struct mb2_tag_mmap *p_mmap = NULL;

struct buffer {
    unsigned long rd_ptr;
    void         *buff;
};

static void read_buffer(struct buffer *buff, char *out, unsigned long count) {
    boot_memcpy(out, ((char *) buff->buff + buff->rd_ptr), count);
}

static int verify_ehdr(const Elf64_Ehdr *ehdr) {
    if (boot_memcmp(&ehdr->e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0) {
        pr_error(pr_fmt("Invalid kernel image header magic"));
        return 1;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        pr_error(pr_fmt("Invalid ELF class"));
        return 1;
    }
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        pr_error(pr_fmt("Invalid ELF data layout"));
        return 1;
    }
    if (ehdr->e_machine != EM_AMD64) {
        pr_error(pr_fmt("Invalid ELF machine"));
        return 1;
    }

    // HACK: ET_EXEC only for now
    if (ehdr->e_type != ET_EXEC) {
        pr_error(pr_fmt("Only ET_EXEC is supported"));
        return 1;
    }
    return 0;
}

static void load_kernel() {
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdrs;

    ehdr = (Elf64_Ehdr *) kernel_blob_start;
    if (verify_ehdr(ehdr) != 0) { hcf(); }

    phdrs = (Elf64_Phdr *) ((char *) kernel_blob_start + ehdr->e_phoff);
}

void boot_main(unsigned long bi) {
    (void) bi;
    boot_serial_init();
    p_mmap = get_mb2_mmap(bi);
    if (!p_mmap) {
        pr_error(pr_fmt("Failed to locate memory map"));
        hcf();
    }

    reserve_boot_regions(bi);
    bump_heap_init(p_mmap);

    load_kernel();

    hcf();
}
