#ifndef _ASI_BITOPS_H
#define _ASI_BITOPS_H

#include <nyx/stddef.h>

static inline int test_bit(int nr, void *addr) {
    return ((1ull << (nr & 63)) & (((const unsigned long long *) addr)[nr >> 6])) != 0;
}

static inline void set_bit(int nr, void *addr) {
    ((unsigned long long *) addr)[nr >> 6] |= (1ull << (nr & 63));
}

static inline void clear_bit(int nr, void *addr) {
    ((unsigned long long *) addr)[nr >> 6] &= ~(1ull << (nr & 63));
}

#define DEFINE_LOAD_LE(name, T)                                                                                        \
    static inline void name(T *out, const unsigned char *in, size_t n_elems) {                                         \
        for (size_t i = 0; i < n_elems; i++) {                                                                         \
            T v = 0;                                                                                                   \
            for (size_t j = 0; j < sizeof(T); j++) v |= (T) ((T) in[i * sizeof(T) + j] << (8 * j));                    \
            out[i] = v;                                                                                                \
        }                                                                                                              \
    }

#define DEFINE_STORE_LE(name, T)                                                                                       \
    static inline void name(unsigned char *out, const T *in, size_t n_elems) {                                         \
        for (size_t i = 0; i < n_elems; i++) {                                                                         \
            T v = in[i];                                                                                               \
            for (size_t j = 0; j < sizeof(T); j++) { out[i * sizeof(T) + j] = (unsigned char) (v >> (8 * j)); }        \
        }                                                                                                              \
    }

DEFINE_LOAD_LE(load_le_u16, u16);
DEFINE_LOAD_LE(load_le_u32, u32);
DEFINE_LOAD_LE(load_le_u64, u64);

DEFINE_STORE_LE(store_le_u16, u16);
DEFINE_STORE_LE(store_le_u32, u32);
DEFINE_STORE_LE(store_le_u64, u64);

#define BIT(n) (1ull << n)

#define __ilog2i(x)  (31 - __builtin_clz(x))
#define __ilog2l(x)  (63 - __builtin_clzl(x))
#define __ilog2ll(x) (63 - __builtin_clzll(x))

#define ilog2(x)  _Generic((x), unsigned int: __ilog2i(x), unsigned long: __ilog2l(x), unsigned long long: __ilog2ll(x))
#define cilog2(x) ((x) <= 1 ? 0 : ilog2((x) - 1) + 1)

#endif
