#include <asi/elf.h>
#include <uapi/nyx/elf.h>

#if ELFCLASS == ELFCLASS64

#define elfhdr     Elf64_Ehdr
#define elf_phdr   Elf64_Phdr
#define elf_shdr   Elf64_Shdr
#define elf_note   Elf64_Note
#define elf_addr_t Elf64_Off
#define Elf_Half   Elf64_Half
#define Elf_Word   Elf64_Word

#else

#define elfhdr     Elf32_Ehdr
#define elf_phdr   Elf32_Phdr
#define elf_shdr   Elf32_Shdr
#define elf_note   Elf32_Note
#define elf_addr_t Elf32_Off
#define Elf_Half   Elf32_Half
#define Elf_Word   Elf32_Word

#endif
