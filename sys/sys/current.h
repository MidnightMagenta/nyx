#ifndef _NYX_CURRENT_H
#define _NYX_CURRENT_H

#include <sys/percpu.h>
#include <sys/proc.h>

static inline struct thread *current() {
    return get_pcpu()->current_task;
}

#endif
