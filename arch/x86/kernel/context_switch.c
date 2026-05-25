#include <mm/vas.h>
#include <nyx/proc.h>
#include <nyx/stddef.h>

extern void switch_to(struct task_struct *prev, struct task_struct *next);

void context_switch(struct task_struct *prev, struct task_struct *next) {
    if (next->vas == NULL) {
        next->active_vas = prev->active_vas;
        vas_grab(next->active_vas);
    } else {
        next->active_vas = next->vas;
        vas_activate(next->vas);
    }

    switch_to(prev, next);
}

void finish_context_switch(struct task_struct *prev, struct task_struct *next) {}
