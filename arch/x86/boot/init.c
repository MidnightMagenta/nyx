#include "boot_utils.h"
#include "elf.h"
#include "printb.h"
#include "region_alloc.h"
#include <asi/page.h>
#include <nyx/util.h>

extern char __kernel_blob_start;
extern char __kernel_blob_end;

void *kernel_blob_start = &__kernel_blob_start;
void *kernel_blob_end   = &__kernel_blob_end;

void __attribute__((noreturn)) die() {
    hcf();
}

#define U64_MAX (~0ULL)

/* KERNEL LOADING */

struct kernel_loadinfo {
    struct memregion k_pregion;
    struct memregion k_vregion;
    u64              k_entry;
};

/* This really should never fail... */
static int verify_ehdr(const Elf64_Ehdr *const ehdr) {
    if (memcmpb(&ehdr->e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0) { return 1; }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) { return 1; }
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) { return 1; }
    if (ehdr->e_type != ET_EXEC) { return 1; } // TODO: ET_DYN support later
    if (ehdr->e_machine != EM_AMD64) { return 1; }
    if (ehdr->e_version != EV_CURRENT) { return 1; }
    return 0;
}

static int get_load_range(const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs, u64 *laddr, u64 *haddr) {
    *laddr = U64_MAX;
    *haddr = 0;

    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdrs[i].p_type != PT_LOAD) { continue; }
        *laddr = min(*laddr, phdrs[i].p_vaddr);
        *haddr = max(*haddr, (phdrs[i].p_vaddr + phdrs[i].p_memsz));
    }

    // really should never fail...
    if (*laddr == U64_MAX) {
        pr_error(pr_fmt("Kernel payload ELF contains no loadable segments"));
        return 1;
    }

    *laddr = ALIGN_DOWN(*laddr, PAGE_SIZE);
    *haddr = ALIGN_UP(*haddr, PAGE_SIZE);

    pr_dbg(pr_fmt("Kernel payload memory range [0x%lx, 0x%lx]"), *laddr, *haddr);

    return 0;
}

static int alloc_kernel_region(u64 laddr, u64 haddr, struct memregion *region) {
    *region = ra_get_region(haddr - laddr, PAGE_SIZE);

    if (region->size == 0) {
        pr_error(pr_fmt("Could not allocate memory for kernel payload"));
        return 1;
    }

    pr_dbg(pr_fmt("Kernel payload physical memory range [0x%lx, 0x%lx]"), region->base, region->base + region->size);

    memsetb((void *) region->base, 0, region->size);

    return 0;
}

static int load_segments(const Elf64_Ehdr *ehdr, const Elf64_Phdr *phdrs, u64 laddr, struct memregion region) {
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        const Elf64_Phdr *ph = &phdrs[i];

        if (ph->p_type != PT_LOAD) { continue; }
        if ((void *) ((char *) ph->p_offset) > kernel_blob_end) {
            pr_error(pr_fmt("Kernel payload offset out of range. p_offset is 0x%lx"), ph->p_offset);
            return 1;
        }
        if (ph->p_memsz < ph->p_filesz) {
            pr_error(pr_fmt("Invalid sections size. p_memsz is 0x%lx p_filesz is 0x%lx"), ph->p_memsz, ph->p_filesz);
            return 1;
        }

        u64 load_base = (ph->p_paddr - laddr) + region.base;
        memcpyb((void *) load_base, (void *) ((char *) kernel_blob_start + ph->p_offset), ph->p_filesz);
    }

    return 0;
}

static int load_kernel(struct kernel_loadinfo *loadinfo) {
    int              res;
    u64              laddr, haddr;
    Elf64_Ehdr      *ehdr;
    Elf64_Phdr      *phdrs;
    struct memregion kernel_region;

    if (!loadinfo) {
        pr_error(pr_fmt("Invalid call to load_kernel()"));
        die();
    }

    ehdr = (Elf64_Ehdr *) kernel_blob_start;
    if ((res = verify_ehdr(ehdr)) != 0) {
        pr_error(pr_fmt("Could not verify kernel payload ELF header"));
        return res;
    }

    phdrs = (Elf64_Phdr *) ((char *) kernel_blob_start + ehdr->e_phoff);

    if ((res = get_load_range(ehdr, phdrs, &laddr, &haddr)) != 0) { return res; }

    if ((res = alloc_kernel_region(laddr, haddr, &kernel_region)) != 0) { return res; }

    if ((res = load_segments(ehdr, phdrs, laddr, kernel_region)) != 0) { return res; }

    memsetb(loadinfo, 0, sizeof(struct kernel_loadinfo));

    loadinfo->k_pregion = kernel_region;
    loadinfo->k_vregion = (struct memregion) {laddr, haddr - laddr};
    loadinfo->k_entry   = (u64) ehdr->e_entry;

    return 0;
}

/* !KERNEL LOADING */

/*
 * This code is responsible for loading the embeded kernel image
 * into a memory location, then mapping that memory, as well as
 * providing a direct map of all physical memory starting at address
 * 0xFFFF888000000000, populate the boot_params structure, and jump
 * to the kernel's entry point.
 */
void boot_main(unsigned long bi) {
    struct kernel_loadinfo kloadinfo;

    (void) bi;
    boot_serial_init();

    if (ra_init(bi) != 0) { die(); }
    if (load_kernel(&kloadinfo) != 0) { die(); }

    pr_dbg(pr_fmt("entry: 0x%lx"), kloadinfo.k_entry);

    hcf();
}
