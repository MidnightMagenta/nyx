#ifndef _NYX_FILEDESC_H
#define _NYX_FILEDESC_H

#include <sys/refcount.h>
#include <sys/spinlock.h>
#include <sys/vfs.h>

#define OPEN_MAX 1024

#define FD_CLOEXEC (1 << 0)
#define FD_CLOFORK (1 << 1)

struct process;

struct fdentry {
    struct file *fe_file;
    int          fe_flags;
};

struct filedesc {
    spinlock_t      fd_lock;
    refcount_t      fd_refs;
    struct fdentry *fd_files;
    int             fd_nfiles;
    struct path     fd_cwd;
    struct path     fd_root;
};

int              fdinit(struct process *p);
struct filedesc *fdcopy(struct filedesc *src);
void             fdfree(struct process *p);

int          fd_install(struct process *p, struct file *fp, int fdflags, int *fdout);
struct file *fget(struct process *p, int fd);
int          fd_close(struct process *p, int fd);
int          fd_dup(struct process *p, int oldfd, int *newfd);

void fd_get_root(struct filedesc *fdp, struct path *out);
void fd_get_cwd(struct filedesc *fdp, struct path *out);
void fd_set_root(struct filedesc *fdp, const struct path *root);
void fd_set_cwd(struct filedesc *fdp, const struct path *cwd);

int kern_open(struct process *p, const char *upath, int flags, mode_t mode, int *fdout);
int kern_close(struct process *p, int fd);
int kern_read(struct process *p, int fd, void *ubuf, size_t count, ssize_t *nread);
int kern_write(struct process *p, int fd, const void *ubuf, size_t count, ssize_t *nwritten);
int kern_dup(struct process *p, int oldfd, int *newfd);

#endif
