#include <mm/memblock.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <nyx/linkage.h>
#include <nyx/types.h>

typedef unsigned long bm_word_t;

#define BMP_WORD_BITS (sizeof(bm_word_t) * 8)

static bm_word_t *bm;

void __init __do_pm_init(const struct memmap *memmap) {}
