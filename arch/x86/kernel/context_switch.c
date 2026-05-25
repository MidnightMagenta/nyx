#include <mm/vas.h>
#include <nyx/proc.h>
#include <nyx/sched.h>
#include <nyx/stddef.h>

extern void switch_to(struct task_struct *prev, struct task_struct *next);

void context_switch(struct task_struct *prev, struct task_struct *next) {
    if (!next->vas) {
        next->active_vas = prev->active_vas;
        vas_grab(next->active_vas);
    } else {
        next->active_vas = next->vas;
        if (prev->active_vas != next->active_vas) { vas_activate(next->vas); }
    }

    switch_to(prev, next);
}

void finish_context_switch(struct task_struct *prev, struct task_struct *next) {
    (void) next;

    if (!prev->vas) { vas_drop(prev->active_vas); }
    prev->active_vas = NULL;

    if (prev->state == DEAD) {
        if (prev->vas) { vas_put(prev->vas); }
        task_free(prev);
    }
}
