#include "boot_utils.h"
#include "elf.h"
#include "printb.h"
#include "region_alloc.h"
#include <nyx/util.h>

extern char __kernel_blob_start;
extern char __kernel_blob_end;

void *kernel_blob_start = &__kernel_blob_start;
void *kernel_blob_end   = &__kernel_blob_end;

int verify_ehdr(const Elf64_Ehdr *const ehdr) {
    if (memcmpb(&ehdr->e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0) { return 1; }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) { return 1; }
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) { return 1; }
    if (ehdr->e_type != ET_EXEC) { return 1; } // TODO: ET_DYN support later
    if (ehdr->e_machine != EM_AMD64) { return 1; }
    if (ehdr->e_version != EV_CURRENT) { return 1; }
    return 0;
}

int load_kernel() {
    int         res;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdrs;

    ehdr = (Elf64_Ehdr *) kernel_blob_start;
    res  = verify_ehdr(ehdr);
    if (res != 0) {
        pr_error(pr_fmt("Could not verify kernel payload ELF header"));
        return res;
    }

    phdrs = (Elf64_Phdr *) ((char *) kernel_blob_start + ehdr->e_phoff);

    return 0;
}

void boot_main(unsigned long bi) {
    (void) bi;
    boot_serial_init();
    ra_init(bi);

    hcf();
}
