#ifndef _ASI_PROC_H
#define _ASI_PROC_H

#include <nyx/compiler.h>
#include <nyx/types.h>

#include <asi/traps.h>

struct thread_context {
    u64 rsp;
} __packed;

struct switchframe {
    u64               r15;
    u64               r14;
    u64               r13;
    u64               r12;
    u64               rbp;
    u64               rbx;
    u64               rip;
    struct trap_frame tf;
};

#endif
