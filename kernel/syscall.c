#include <nyx/syscall.h>

#define EXTERN_SYSCALL(name) extern int name(struct thread *, struct syscall_args *, register_t *)

EXTERN_SYSCALL(sys_fork);
EXTERN_SYSCALL(sys_exit);

syscall_fn syscall_table[]    = {sys_fork, sys_exit};
size_t     syscall_table_size = ARRAY_SIZE(syscall_table);
