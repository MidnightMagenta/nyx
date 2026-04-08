#ifndef _NYX_BITMAP_H
#define _NYX_BITMAP_H

#include <nyx/stddef.h>
#include <nyx/types.h>

typedef u64 bitmap_word_t;

#define BITMAP_WORD_BITS    (sizeof(bitmap_word_t) * 8)
#define __BITMAP_WORD_SHIFT (__builtin_ctz(BITMAP_WORD_BITS))
#define __BITMAP_WORD_MASK  (BITMAP_WORD_BITS - 1)
#define __BITMAP_ONE        ((bitmap_word_t) 1)

#define __BITS_TO_WORDS(bits) (((bits) + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS)

#define DECLARE_STATIC_BITMAP(name, bits) bitmap_word_t name[__BITS_TO_WORDS(bits)]
#define DECLARE_DYNAMIC_BITMAP(name)      bitmap_word_t *name;

static inline void bm_set(bitmap_word_t *bitmap, size_t bit) {
    bitmap[bit >> __BITMAP_WORD_SHIFT] |= (__BITMAP_ONE << (bit & __BITMAP_WORD_MASK));
}

static inline void bm_clear(bitmap_word_t *bitmap, size_t bit) {
    bitmap[bit >> __BITMAP_WORD_SHIFT] &= ~(__BITMAP_ONE << (bit & __BITMAP_WORD_MASK));
}

static inline void bm_flip(bitmap_word_t *bitmap, size_t bit) {
    bitmap[bit >> __BITMAP_WORD_SHIFT] ^= (__BITMAP_ONE << (bit & __BITMAP_WORD_MASK));
}

static inline int bm_get(bitmap_word_t *bitmap, size_t bit) {
    return ((bitmap[bit >> __BITMAP_WORD_SHIFT] >> (bit & __BITMAP_WORD_MASK)) & __BITMAP_ONE);
}

#endif
