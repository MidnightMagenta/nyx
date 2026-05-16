#ifndef _ASI_PROC_H
#define _ASI_PROC_H

#include <nyx/compiler.h>
#include <nyx/types.h>

struct proc_context {
    u64 rsp;
} __packed;

#endif
