#include <sys/compiler.h>
#include <sys/current.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>
#include <sys/fs/cpio.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/linkage.h>
#include <sys/mm/mm_types.h>
#include <sys/mm/virtmem.h>
#include <sys/mm/vmspace.h>
#include <sys/panic.h>
#include <sys/printk.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vfs.h>

#include <asi/address.h>
#include <asi/bootparam.h>
#include <asi/bug.h>
#include <asi/irq.h>
#include <asi/traps.h>

#ifdef CONFIG_KERNEL_TESTS
extern void __do_kernel_tests();
#else
#define __do_kernel_tests()
#endif

static void __noreturn __idle_task_fn() {
    for (;;) { asm volatile("hlt"); }
}

extern void setup_arch();
extern void init_memory();
extern void init_irq();
extern void init_timer();
extern void init_sched();
extern void init_vfs();
extern void init_filesystems();
extern void do_initcalls();
extern void reaper(void *arg);
void        init_proc(void *arg);

extern struct thread proc0;
struct thread       *initproc;

static __init void mount_root() {
    struct path root;

    BUG_ON(vfs_mount_root("ramfs", "rootfs", 0, NULL));

    fd_get_root(current()->t_proc->p_fd, &root);
    BUG_ON(cpio_unpack(__va(get_initramfs()), get_initramfs_len(), &root));
    path_put(&root);
}

static void __init mount_devfs() {
    int res = vfs_mkdir(NULL, "/dev", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    BUG_ON(res && res != -EEXIST);
    BUG_ON(vfs_mount("ramfs", "devfs", "/dev", 0, NULL));
}

static __init void start_init() {
    struct file *console;
    int          stdinfd, stdoutfd, stderrfd;
    if (do_fork(&proc0, FORK_NOZOMBIE | FORK_SHAREVM | FORK_SHAREFD, &init_proc, NULL, NULL, &initproc) != 0) {
        panic("failed to start init");
    }

    BUG_ON(initproc->t_proc->p_pid != 1);
    strncpy(initproc->t_proc->p_name, "init", PROC_NAME_LEN);

    BUG_ON(vfs_open(NULL, "/dev/console", O_RDWR, 0, &console));
    BUG_ON(fd_install(initproc->t_proc, console, 0, &stdinfd));
    BUG_ON(kern_dup(initproc->t_proc, stdinfd, &stdoutfd));
    BUG_ON(kern_dup(initproc->t_proc, stdinfd, &stderrfd));
    BUG_ON(stdinfd != 0 || stdoutfd != 1 || stderrfd != 2);
}

void __init start_kernel() {
    pr_info(log_generic, "kernel build ID: %s\n", NYX_BUILD_ID);

    setup_arch();
    init_memory();
    init_irq();
    init_vfs();
    init_filesystems();
    init_timer();
    init_sched();
    mount_root();
    mount_devfs();
    do_initcalls();

    __do_kernel_tests();

    pr_info(log_generic, "finish\n");

    start_init();
    kthread_create(reaper, NULL, "procreaper");

    arch_irq_enable();

    __idle_task_fn();

    BUG();
}

void __init init_proc(void *arg) {
    (void) arg;
    do_exec("/sbin/init", NULL, NULL);
    return;
}
