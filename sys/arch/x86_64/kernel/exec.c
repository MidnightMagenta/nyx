#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/uapi/nyx/elf.h>

int __arch_exec_elf_verify_ident(const unsigned char *ident) {
    if (ident[EI_CLASS] != ELFCLASS64) { return -ENOEXEC; }
    if (ident[EI_DATA] != ELFDATA2LSB) { return -ENOEXEC; }

    return 0;
}
