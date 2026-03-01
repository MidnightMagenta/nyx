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
#include <nyx/types.h>

#define MB2_MMAP_AVAILABLE        1
#define MB2_MMAP_RESERVED         2
#define MB2_MMAP_ACPI_RECLAIMABLE 3
#define MB2_MMAP_NVS              4
#define MB2_MMAP_BADRAM           5

struct mb2_mmap_entry {
    u64 addr;
    u64 len;
    u32 type;
    u32 zero;
} __packed;

struct mb2_color {
    u8 red;
    u8 green;
    u8 blue;
} __packed;

struct mb2_tag {
    u32 type;
    u32 size;
} __packed;

struct mb2_tag_string {
    u32  type;
    u32  size;
    char str[0];
} __packed;

struct mb2_tag_basic_meminfo {
    u32 type;
    u32 size;
    u32 mem_lower;
    u32 mem_upper;
} __packed;

struct mb2_tag_bootdev {
    u32 type;
    u32 size;
    u32 biosdev;
    u32 slice;
    u32 port;
} __packed;

struct mb2_tag_mmap {
    u32                   type;
    u32                   size;
    u32                   entry_size;
    u32                   entry_version;
    struct mb2_mmap_entry entries[0];
} __packed;

struct mb2_vbe_info_block {
    u8 external_specification[512];
} __packed;

struct mb2_vbe_mode_info_block {
    u8 external_specification[512];
} __packed;

struct mb2_tag_vbe {
    u32                            type;
    u32                            size;
    u16                            vbe_mode;
    u16                            vbe_interface_seg;
    u16                            vbe_interface_off;
    u16                            vbe_interface_len;
    struct mb2_vbe_info_block      vbe_control_info;
    struct mb2_vbe_mode_info_block vbe_mode_info;
} __packed;

#define MB2_FB_TYPE_INDEXED  0
#define MB2_FB_TYPE_RGB      1
#define MB2_FB_TYPE_EGA_TEXT 2

struct mb2_tag_fb_common {
    u32 type;
    u32 size;
    u64 fb_addr;
    u64 fb_pitch;
    u64 fb_width;
    u64 fb_height;
    u8  fb_bpp;
    u8  fb_type;
    u16 reserved;
} __packed;

struct mb2_tag_fb {
    struct mb2_tag_fb_common common;

    union {
        struct {
            u16              fb_palette_num_colors;
            struct mb2_color fb_palette[0];
        } __packed;
        struct {
            u8 fb_red_field_pos;
            u8 fb_red_mask_size;
            u8 fb_green_field_pos;
            u8 fb_green_mask_size;
            u8 fb_blue_field_pos;
            u8 fb_blue_mask_size;
        } __packed;
    };
} __packed;

struct mb2_tag_elf_sections {
    u32  type;
    u32  size;
    u32  num;
    u32  entsize;
    u32  shndx;
    char sections[0];
} __packed;

struct mb2_tag_apm {
    u32 type;
    u32 size;
    u16 version;
    u16 cseg;
    u32 offset;
    u16 cseg_16;
    u16 dseg;
    u16 flags;
    u16 cseg_len;
    u16 cseg_16_len;
    u16 dseg_len;
} __packed;

struct mb2_tag_efi32 {
    u32 type;
    u32 size;
    u32 ptr;
} __packed;

struct mb2_tag_efi64 {
    u32 type;
    u32 size;
    u64 ptr;
} __packed;

struct mb2_tag_smbios {
    u32 type;
    u32 size;
    u8  major;
    u8  minor;
    u8  reserved[6];
    u8  tables[0];
} __packed;

struct mb2_tag_acpi1 {
    u32 type;
    u32 size;
    u8  rsdp[0];
} __packed;

struct mb2_tag_acpi2 {
    u32 type;
    u32 size;
    u8  rsdp[0];
} __packed;

struct mb2_tag_net {
    u32 type;
    u32 size;
    u8  dhcpack[0];
} __packed;

struct mb2_tag_efi_mmap {
    u32 type;
    u32 size;
    u32 desc_size;
    u32 desc_vers;
    u8  efi_mmap[0];
} __packed;

struct mb2_tag_efi32_ih {
    u32 type;
    u32 size;
    u32 ptr;
} __packed;

struct mb2_tag_efi64_ih {
    u32 type;
    u32 size;
    u64 ptr;
} __packed;

struct mb2_tag_load_base_addr {
    u32 type;
    u32 size;
    u32 load_base_addr;
} __packed;

#endif

#endif
