#ifndef _NYX_MUTEX_H
#define _NYX_MUTEX_H

#include <sys/spinlock.h>
#include <sys/stddef.h>

struct thread;

struct mutex {
    spinlock_t     m_lock;
    bool           m_locked;
    unsigned int   m_waiters;
    struct thread *m_owner;
};

static inline void mutex_init(struct mutex *m) {
    spinlock_init(&m->m_lock);
    m->m_locked  = false;
    m->m_waiters = 0;
    m->m_owner   = NULL;
}

void mutex_lock(struct mutex *m);
bool mutex_trylock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif
