#ifndef _UAPI_ASI_GENERIC_ELF_TYPES_H
#define _UAPI_ASI_GENERIC_ELF_TYPES_H

typedef unsigned short Elf32_Half;
typedef unsigned int   Elf32_Word;
typedef int            Elf32_Sword;
typedef unsigned int   Elf32_Addr;
typedef unsigned int   Elf32_Off;

typedef unsigned short    Elf64_Half;
typedef unsigned int      Elf64_Word;
typedef int               Elf64_Sword;
typedef unsigned long int Elf64_Xword;
typedef long int          Elf64_Sxword;
typedef unsigned long int Elf64_Addr;
typedef unsigned long int Elf64_Off;

#endif
