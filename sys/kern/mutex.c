#include <sys/current.h>
#include <sys/mutex.h>
#include <sys/wait.h>

#include <asi/bug.h>

void mutex_lock(struct mutex *m) {
    for (;;) {
        spin_lock(&m->m_lock);
        if (!m->m_locked) {
            m->m_locked = true;
            m->m_owner  = current();
            spin_unlock(&m->m_lock);
            return;
        }

        BUG_ON(m->m_owner && m->m_owner == current());

        m->m_waiters++;
        sleep_setup(m, "mutex");
        spin_unlock(&m->m_lock);
        sleep_finish(1);

        spin_lock(&m->m_lock);
        m->m_waiters--;
        spin_unlock(&m->m_lock);
    }
}

bool mutex_trylock(struct mutex *m) {
    bool acquired = false;

    spin_lock(&m->m_lock);
    if (!m->m_locked) {
        m->m_locked = true;
        m->m_owner  = current();
        acquired    = true;
    }
    spin_unlock(&m->m_lock);

    return acquired;
}

void mutex_unlock(struct mutex *m) {
    spin_lock(&m->m_lock);
    BUG_ON(!m->m_locked);
    m->m_locked = false;
    m->m_owner  = NULL;
    if (m->m_waiters) { wakeup(m); }
    spin_unlock(&m->m_lock);
}
