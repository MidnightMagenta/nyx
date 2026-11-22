#ifndef _NYX_SPINLOCK_H
#define _NYX_SPINLOCK_H

#include <sys/atomic.h>
#include <sys/types.h>

#include <asi/irq.h>

typedef struct spinlock {
    atomic_bool_t __locked;
    flags_t       __old_flags;
} spinlock_t;

static inline void spinlock_init(spinlock_t *l) {
    atomic_clear(&l->__locked, ATOMIC_SEQ_CST);
}

static inline void spin_lock(spinlock_t *l) {
    flags_t flags = arch_irq_save();
    while (atomic_test_and_set(&l->__locked, ATOMIC_ACQUIRE)) { barrier(); }
    l->__old_flags = flags;
}

static inline void spin_unlock(spinlock_t *l) {
    flags_t flags = l->__old_flags;
    atomic_clear(&l->__locked, ATOMIC_RELEASE);
    arch_irq_restore(flags);
}

#endif
