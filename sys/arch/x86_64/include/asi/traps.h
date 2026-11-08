#ifndef _ASI_ISR_CONTEXT_H
#define _ASI_ISR_CONTEXT_H

#include <nyx/compiler.h>
#include <nyx/types.h>

#include <asi/cpu.h>

struct interrupt_frame {
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __packed;

struct trap_frame {
    struct regs            regs;
    u64                    vector;
    u64                    ecode;
    struct interrupt_frame frame;
} __packed;

#endif
