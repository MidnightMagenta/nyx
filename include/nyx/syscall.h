#ifndef _NYX_SYSCALL_H
#define _NYX_SYSCALL_H

#include <nyx/proc.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

struct syscall_args {
    unsigned long arg1, arg2, arg3, arg4, arg5, arg6;
};

typedef int (*syscall_fn)(struct thread *, struct syscall_args *, register_t *);


extern syscall_fn syscall_table[];
extern size_t     syscall_table_size;

#endif
