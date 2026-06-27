#include <nyx/syscall.h>
#include <uapi/syscall.h>

#define EXTERN_SYSCALL(name) extern int name(struct thread *, struct syscall_args *, register_t *)

EXTERN_SYSCALL(sys_fork);
EXTERN_SYSCALL(sys_vfork);
EXTERN_SYSCALL(sys_exit);
EXTERN_SYSCALL(sys_wait3);

syscall_fn syscall_table[] = {
        [SYS_fork]  = sys_fork,
        [SYS_vfork] = sys_vfork,
        [SYS_exit]  = sys_exit,
        [SYS_wait]  = sys_wait3,
};

size_t syscall_table_size = ARRAY_SIZE(syscall_table);
