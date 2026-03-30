#include <mm/memblock.h>
#include <mm/pmm.h>
#include <nyx/align.h>
#include <nyx/kernel.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <nyx/printk.h>
#include <nyx/types.h>

#define pr_fmt(fmt) "pmm: " fmt "\n"

void __init init_page_alloc() {}
