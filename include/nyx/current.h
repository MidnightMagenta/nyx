#ifndef _NYX_CURRENT_H
#define _NYX_CURRENT_H

#include <nyx/percpu.h>
#include <nyx/proc.h>

static inline struct thread *current() {
    return get_pcpu()->current_task;
}

#endif
