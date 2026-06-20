#ifndef _MM_MM_H
#define _MM_MM_H

#include <mm/mm_types.h>
#include <nyx/atomic.h>
#include <nyx/proc.h>

struct vmspace *vmspace_fork(struct process *p);

#endif
