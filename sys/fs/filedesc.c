#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>
#include <sys/minmax.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/mm_types.h>
#include <sys/mm/slab.h>
#include <sys/mm/vmspace.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/vfs.h>

#include <asi/bug.h>
#include <asi/page.h>

#include "internal.h"

#define NDFILE      16
#define BOUNCE_SIZE PAGE_SIZE

static kmem_cache_t *filedesc_cache;

void filedesc_init() {
    filedesc_cache =
            kmem_create_cache("filedesc", sizeof(struct filedesc), _Alignof(struct filedesc), NULL, NULL, M_SLEEPOK);
    BUG_ON(!filedesc_cache);
}

static struct fdentry *fdarray_alloc(int n) {
    struct fdentry *arr = kmalloc(sizeof(struct fdentry) * (size_t) n, M_SLEEPOK);
    if (arr) { memset(arr, 0, sizeof(struct fdentry) * (size_t) n); }
    return arr;
}

static struct filedesc *filedesc_alloc(int nfiles) {
    struct filedesc *fdp = kmem_cache_alloc(filedesc_cache, M_SLEEPOK);
    if (!fdp) { return NULL; }

    memset(fdp, 0, sizeof(struct filedesc));
    fdp->fd_files = fdarray_alloc(nfiles);
    if (!fdp->fd_files) {
        kmem_cache_free(filedesc_cache, fdp);
        return NULL;
    }

    fdp->fd_nfiles = nfiles;
    spinlock_init(&fdp->fd_lock);
    refcount_init(&fdp->fd_refs, 1);

    return fdp;
}

int fdinit(struct process *p) {
    p->p_fd = filedesc_alloc(NDFILE);
    return p->p_fd ? 0 : -ENOMEM;
}

struct filedesc *fdcopy(struct filedesc *src) {
    struct filedesc *fdp;
    int              n;

    spin_lock(&src->fd_lock);
    n = src->fd_nfiles;
    spin_unlock(&src->fd_lock);

    for (;;) {
        fdp = filedesc_alloc(n);
        if (!fdp) { return NULL; }

        spin_lock(&src->fd_lock);
        if (src->fd_nfiles == n) { break; }
        n = src->fd_nfiles;
        spin_unlock(&src->fd_lock);

        kfree(fdp->fd_files);
        kmem_cache_free(filedesc_cache, fdp);
    }

    for (int i = 0; i < n; i++) {
        struct fdentry *fe = &src->fd_files[i];

        if (!fe->fe_file || (fe->fe_flags & FD_CLOFORK)) { continue; }

        fhold(fe->fe_file);
        fdp->fd_files[i] = *fe;
    }

    fdp->fd_cwd  = src->fd_cwd;
    fdp->fd_root = src->fd_root;
    if (fdp->fd_cwd.p_mnt) { path_get(&fdp->fd_cwd); }
    if (fdp->fd_root.p_mnt) { path_get(&fdp->fd_root); }

    spin_unlock(&src->fd_lock);

    return fdp;
}

void fdfree(struct process *p) {
    struct filedesc *fdp = p->p_fd;

    if (!fdp) { return; }
    p->p_fd = NULL;

    if (!refcount_dec_and_test(&fdp->fd_refs)) { return; }

    for (int i = 0; i < fdp->fd_nfiles; i++) {
        if (fdp->fd_files[i].fe_file) { fput(fdp->fd_files[i].fe_file); }
    }

    if (fdp->fd_cwd.p_mnt) { path_put(&fdp->fd_cwd); }
    if (fdp->fd_root.p_mnt) { path_put(&fdp->fd_root); }

    kfree(fdp->fd_files);
    kmem_cache_free(filedesc_cache, fdp);
}

static int fdgrow(struct filedesc *fdp, int n) {
    struct fdentry *arr, *old;
    int             newn;

    if (n >= OPEN_MAX) { return -EMFILE; }

    newn = MIN(n * 2, OPEN_MAX);
    arr  = fdarray_alloc(newn);
    if (!arr) { return -ENOMEM; }

    spin_lock(&fdp->fd_lock);
    if (fdp->fd_nfiles == n) {
        memcpy(arr, fdp->fd_files, sizeof(struct fdentry) * (size_t) n);
        old            = fdp->fd_files;
        fdp->fd_files  = arr;
        fdp->fd_nfiles = newn;
    } else {
        old = arr;
    }
    spin_unlock(&fdp->fd_lock);

    kfree(old);
    return 0;
}

int fd_install(struct process *p, struct file *fp, int fdflags, int *fdout) {
    struct filedesc *fdp = p->p_fd;
    int              n, res;

    for (;;) {
        spin_lock(&fdp->fd_lock);
        for (int i = 0; i < fdp->fd_nfiles; i++) {
            if (!fdp->fd_files[i].fe_file) {
                fdp->fd_files[i].fe_file  = fp;
                fdp->fd_files[i].fe_flags = fdflags;
                spin_unlock(&fdp->fd_lock);
                *fdout = i;
                return 0;
            }
        }
        n = fdp->fd_nfiles;
        spin_unlock(&fdp->fd_lock);

        if ((res = fdgrow(fdp, n))) { return res; }
    }
}

struct file *fget(struct process *p, int fd) {
    struct filedesc *fdp = p->p_fd;
    struct file     *fp  = NULL;

    spin_lock(&fdp->fd_lock);
    if (fd >= 0 && fd < fdp->fd_nfiles && fdp->fd_files[fd].fe_file) {
        fp = fdp->fd_files[fd].fe_file;
        fhold(fp);
    }
    spin_unlock(&fdp->fd_lock);

    return fp;
}

int fd_close(struct process *p, int fd) {
    struct filedesc *fdp = p->p_fd;
    struct file     *fp  = NULL;

    spin_lock(&fdp->fd_lock);
    if (fd >= 0 && fd < fdp->fd_nfiles) {
        fp                        = fdp->fd_files[fd].fe_file;
        fdp->fd_files[fd].fe_file = NULL;
    }
    spin_unlock(&fdp->fd_lock);

    if (!fp) { return -EBADF; }

    fput(fp);
    return 0;
}

int fd_dup(struct process *p, int oldfd, int *newfd) {
    struct file *fp = fget(p, oldfd);
    int          res;

    if (!fp) { return -EBADF; }
    if ((res = fd_install(p, fp, 0, newfd))) { fput(fp); }

    return res;
}

static void fd_get_path(struct filedesc *fdp, struct path *src, struct path *out) {
    spin_lock(&fdp->fd_lock);
    *out = *src;
    if (out->p_mnt) { path_get(out); }
    spin_unlock(&fdp->fd_lock);
}

static void fd_set_path(struct filedesc *fdp, struct path *dst, const struct path *src) {
    struct path old;
    struct path new = *src;

    path_get(&new);

    spin_lock(&fdp->fd_lock);
    old  = *dst;
    *dst = new;
    spin_unlock(&fdp->fd_lock);

    if (old.p_mnt) { path_put(&old); }
}

void fd_get_root(struct filedesc *fdp, struct path *out) {
    fd_get_path(fdp, &fdp->fd_root, out);
}

void fd_get_cwd(struct filedesc *fdp, struct path *out) {
    fd_get_path(fdp, &fdp->fd_cwd, out);
}

void fd_set_root(struct filedesc *fdp, const struct path *root) {
    fd_set_path(fdp, &fdp->fd_root, root);
}

void fd_set_cwd(struct filedesc *fdp, const struct path *cwd) {
    fd_set_path(fdp, &fdp->fd_cwd, cwd);
}

int kern_open(struct process *p, const char *upath, int flags, mode_t mode, int *fdout) {
    struct file *fp;
    char        *kpath;
    int          fdflags = 0;
    int          res;

    kpath = kmalloc(PATH_MAX, M_SLEEPOK);
    if (!kpath) { return -ENOMEM; }

    if ((res = copyinstr(kpath, upath, PATH_MAX, NULL))) { goto out; }
    if ((res = vfs_open(NULL, kpath, flags, mode, &fp))) { goto out; }

    if (flags & O_CLOEXEC) { fdflags |= FD_CLOEXEC; }
    if (flags & O_CLOFORK) { fdflags |= FD_CLOFORK; }
    if ((res = fd_install(p, fp, fdflags, fdout))) { fput(fp); }

out:
    kfree(kpath);
    return res;
}

int kern_close(struct process *p, int fd) {
    return fd_close(p, fd);
}

int kern_read(struct process *p, int fd, void *ubuf, size_t count, ssize_t *nread) {
    struct file *fp;
    char        *kbuf;
    size_t       done = 0;
    int          res  = 0;

    if (!(fp = fget(p, fd))) { return -EBADF; }
    if (count == 0) { goto out; }

    kbuf = kmalloc(MIN(count, BOUNCE_SIZE), M_SLEEPOK);
    if (!kbuf) {
        res = -ENOMEM;
        goto out;
    }

    while (done < count) {
        size_t  chunk = MIN(count - done, BOUNCE_SIZE);
        ssize_t n     = vfs_read(fp, kbuf, chunk);

        if (n < 0) {
            if (done == 0) { res = (int) n; }
            break;
        }
        if (n > 0 && copyout((char *) ubuf + done, kbuf, (size_t) n)) {
            res = -EFAULT;
            break;
        }

        done += (size_t) n;
        if ((size_t) n < chunk) { break; }
    }

    kfree(kbuf);

out:
    fput(fp);
    *nread = (ssize_t) done;
    return res;
}

int kern_write(struct process *p, int fd, const void *ubuf, size_t count, ssize_t *nwritten) {
    struct file *fp;
    char        *kbuf;
    size_t       done = 0;
    int          res  = 0;

    if (!(fp = fget(p, fd))) { return -EBADF; }
    if (count == 0) { goto out; }

    kbuf = kmalloc(MIN(count, BOUNCE_SIZE), M_SLEEPOK);
    if (!kbuf) {
        res = -ENOMEM;
        goto out;
    }

    while (done < count) {
        size_t  chunk = MIN(count - done, BOUNCE_SIZE);
        ssize_t n;

        if (copyin(kbuf, (const char *) ubuf + done, chunk)) {
            if (done == 0) { res = -EFAULT; }
            break;
        }

        n = vfs_write(fp, kbuf, chunk);
        if (n < 0) {
            if (done == 0) { res = (int) n; }
            break;
        }

        done += (size_t) n;
        if ((size_t) n < chunk) { break; }
    }

    kfree(kbuf);

out:
    fput(fp);
    *nwritten = (ssize_t) done;
    return res;
}

int kern_dup(struct process *p, int oldfd, int *newfd) {
    return fd_dup(p, oldfd, newfd);
}

int sys_open(struct thread *t, struct syscall_args *args, register_t *retval) {
    int fd  = -1;
    int res = kern_open(t->t_proc, (const char *) args->arg1, (int) args->arg2, (mode_t) args->arg3, &fd);

    if (!res) { *retval = fd; }
    return res;
}

int sys_close(struct thread *t, struct syscall_args *args, register_t *retval) {
    (void) retval;
    return kern_close(t->t_proc, (int) args->arg1);
}

int sys_read(struct thread *t, struct syscall_args *args, register_t *retval) {
    ssize_t n   = 0;
    int     res = kern_read(t->t_proc, (int) args->arg1, (void *) args->arg2, (size_t) args->arg3, &n);

    *retval = n;
    return res;
}

int sys_write(struct thread *t, struct syscall_args *args, register_t *retval) {
    ssize_t n   = 0;
    int     res = kern_write(t->t_proc, (int) args->arg1, (const void *) args->arg2, (size_t) args->arg3, &n);

    *retval = n;
    return res;
}

int sys_dup(struct thread *t, struct syscall_args *args, register_t *retval) {
    int fd  = -1;
    int res = kern_dup(t->t_proc, (int) args->arg1, &fd);

    if (!res) { *retval = fd; }
    return res;
}
