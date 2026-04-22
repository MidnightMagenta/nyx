#ifndef _ASI_BITOPS_H
#define _ASI_BITOPS_H

static inline int test_bit(int nr, void *addr) {
    return ((1ull << (nr & 63)) & (((const unsigned long long *) addr)[nr >> 6])) != 0;
}

static inline void set_bit(int nr, void *addr) {
    ((unsigned long long *) addr)[nr >> 6] |= (1ull << (nr & 63));
}

static inline void clear_bit(int nr, void *addr) {
    ((unsigned long long *) addr)[nr >> 6] &= ~(1ull << (nr & 63));
}

#endif
