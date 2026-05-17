#ifndef _NYX_ATOMIC_H
#define _NYX_ATOMIC_H

typedef struct {
    int d;
} atomic_t;

#define atomic_set(a, v)                                                                                               \
    do { (a)->d = (v); } while (0)

#endif
