#ifndef _ASI_ISR_CONTEXT_H
#define _ASI_ISR_CONTEXT_H

#include <nyx/compiler.h>
#include <nyx/types.h>

#include <asi/cpu.h>

struct trap_frame {
    struct gp_regs regs;
    u64            vector;
    u64            ecode;
    u64            rip;
    u64            cs;
    u64            rflags;
    u64            rsp;
    u64            ss;
} __packed;

#endif
