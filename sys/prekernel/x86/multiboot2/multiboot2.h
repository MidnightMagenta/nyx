#ifndef _MB2_H
#define _MB2_H

#define MB2_HEADER_MAGIC     0xe85250d6
#define MB2_BOOTLOADER_MAGIC 0x36d76289
#define MB2_ARCH_i386        0

// multiboot header tag defines
#define MB2_HEADER_TAG_END                 0
#define MB2_HEADER_TAG_INFORMATION_REQUEST 1
#define MB2_HEADER_TAG_ADDRESS             2
#define MB2_HEADER_TAG_ENTRY_ADDRESS       3
#define MB2_HEADER_TAG_CONSOLE_FLAGS       4
#define MB2_HEADER_TAG_FRAMEBUFFER         5
#define MB2_HEADER_TAG_MODULE_ALIGN        6
#define MB2_HEADER_TAG_EFI_BS              7
#define MB2_HEADER_TAG_ENTRY_ADDRESS_EFI32 8
#define MB2_HEADER_TAG_ENTRY_ADDRESS_EFI64 9
#define MB2_HEADER_TAG_RELOCATABLE         10

// mutliboot tag defines
#define MB2_TAG_END             0
#define MB2_TAG_CMDLINE         1
#define MB2_TAG_BOOTLAODER_NAME 2
#define MB2_TAG_MODULE          3
#define MB2_TAG_BASIC_MEMINFO   4
#define MB2_TAG_BOOTDEV         5
#define MB2_TAG_MMAP            6
#define MB2_TAG_VBE             7
#define MB2_TAG_FB              8
#define MB2_TAG_ELF_SECTIONS    9
#define MB2_TAG_APM             10
#define MB2_TAG_EFI32           11
#define MB2_TAG_EFI64           12
#define MB2_TAG_SMBIOS          13
#define MB2_TAG_ACPI1           14
#define MB2_TAG_ACPI2           15
#define MB2_TAG_NET             16
#define MB2_TAG_EFI_MMAP        17
#define MB2_TAG_EFI_BS          18
#define MB2_TAG_EFI32_IH        19
#define MB2_TAG_EFI64_IH        20
#define MB2_TAG_LOAD_BASE_ADDR  21

#ifndef __ASSEMBLY__

#include <nyx/compiler.h>

#define MB2_MMAP_AVAILABLE        1
#define MB2_MMAP_RESERVED         2
#define MB2_MMAP_ACPI_RECLAIMABLE 3
#define MB2_MMAP_NVS              4
#define MB2_MMAP_BADRAM           5

struct mb2_mmap_entry {
    unsigned long addr;
    unsigned long len;
    unsigned int  type;
    unsigned int  zero;
} __packed;

struct mb2_color {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} __packed;

struct mb2_tag {
    unsigned int type;
    unsigned int size;
} __packed;

struct mb2_tag_string {
    unsigned int type;
    unsigned int size;
    char         str[];
} __packed;

struct mb2_tag_module {
    unsigned int  type;
    unsigned int  size;
    unsigned int  mod_start;
    unsigned int  mod_end;
    unsigned char str[];
};

struct mb2_tag_basic_meminfo {
    unsigned int type;
    unsigned int size;
    unsigned int mem_lower;
    unsigned int mem_upper;
} __packed;

struct mb2_tag_bootdev {
    unsigned int type;
    unsigned int size;
    unsigned int biosdev;
    unsigned int slice;
    unsigned int port;
} __packed;

struct mb2_tag_mmap {
    unsigned int          type;
    unsigned int          size;
    unsigned int          entry_size;
    unsigned int          entry_version;
    struct mb2_mmap_entry entries[];
} __packed;

struct mb2_vbe_info_block {
    unsigned char external_specification[512];
} __packed;

struct mb2_vbe_mode_info_block {
    unsigned char external_specification[512];
} __packed;

struct mb2_tag_vbe {
    unsigned int                   type;
    unsigned int                   size;
    unsigned short                 vbe_mode;
    unsigned short                 vbe_interface_seg;
    unsigned short                 vbe_interface_off;
    unsigned short                 vbe_interface_len;
    struct mb2_vbe_info_block      vbe_control_info;
    struct mb2_vbe_mode_info_block vbe_mode_info;
} __packed;

#define MB2_FB_TYPE_INDEXED  0
#define MB2_FB_TYPE_RGB      1
#define MB2_FB_TYPE_EGA_TEXT 2

struct mb2_tag_fb {
    unsigned int  type;
    unsigned int  size;
    unsigned long fb_addr;
    unsigned int  fb_pitch;
    unsigned int  fb_width;
    unsigned int  fb_height;
    unsigned char fb_bpp;
    unsigned char fb_type;
    unsigned char reserved;

    union {
        struct {
            unsigned short   fb_palette_num_colors;
            struct mb2_color fb_palette[];
        } __packed;
        struct {
            unsigned char fb_red_field_pos;
            unsigned char fb_red_mask_size;
            unsigned char fb_green_field_pos;
            unsigned char fb_green_mask_size;
            unsigned char fb_blue_field_pos;
            unsigned char fb_blue_mask_size;
        } __packed;
    };
} __packed;

struct mb2_tag_elf_sections {
    unsigned int type;
    unsigned int size;
    unsigned int num;
    unsigned int entsize;
    unsigned int shndx;
    char         sections[];
} __packed;

struct mb2_tag_apm {
    unsigned int   type;
    unsigned int   size;
    unsigned short version;
    unsigned short cseg;
    unsigned int   offset;
    unsigned short cseg_16;
    unsigned short dseg;
    unsigned short flags;
    unsigned short cseg_len;
    unsigned short cseg_16_len;
    unsigned short dseg_len;
} __packed;

struct mb2_tag_efi32 {
    unsigned int type;
    unsigned int size;
    unsigned int ptr;
} __packed;

struct mb2_tag_efi64 {
    unsigned int  type;
    unsigned int  size;
    unsigned long ptr;
} __packed;

struct mb2_tag_smbios {
    unsigned int  type;
    unsigned int  size;
    unsigned char major;
    unsigned char minor;
    unsigned char reserved[6];
    unsigned char tables[];
} __packed;

struct mb2_tag_acpi1 {
    unsigned int  type;
    unsigned int  size;
    unsigned char rsdp[];
} __packed;

struct mb2_tag_acpi2 {
    unsigned int  type;
    unsigned int  size;
    unsigned char rsdp[];
} __packed;

struct mb2_tag_net {
    unsigned int  type;
    unsigned int  size;
    unsigned char dhcpack[];
} __packed;

struct mb2_tag_efi_mmap {
    unsigned int  type;
    unsigned int  size;
    unsigned int  desc_size;
    unsigned int  desc_vers;
    unsigned char efi_mmap[];
} __packed;

struct mb2_tag_efi32_ih {
    unsigned int type;
    unsigned int size;
    unsigned int ptr;
} __packed;

struct mb2_tag_efi64_ih {
    unsigned int  type;
    unsigned int  size;
    unsigned long ptr;
} __packed;

struct mb2_tag_load_base_addr {
    unsigned int type;
    unsigned int size;
    unsigned int load_base_addr;
} __packed;

#endif

#endif
