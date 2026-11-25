#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/mm/mm_types.h>
#include <sys/mm/slab.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#include "internal.h"

static kmem_cache_t *file_cache;

void file_init() {
    file_cache = kmem_create_cache("file", sizeof(struct file), _Alignof(struct file), NULL, NULL, M_SLEEPOK);
    BUG_ON(!file_cache);
}

static unsigned long flags_to_fmode(int flags) {
    switch (flags & O_ACCMODE) {
        case O_RDONLY:
            return FMODE_READ;
        case O_WRONLY:
            return FMODE_WRITE;
        case O_RDWR:
            return FMODE_READ | FMODE_WRITE;
        default:
            return 0;
    }
}

int vfs_open_path(struct path *p, int flags, struct file **out) {
    struct inode *inode = p->p_dent->d_inode;
    struct file  *fp;
    int           res;

    if (S_ISLNK(inode->i_mode)) {
        res = -ELOOP;
        goto fail0;
    }
    if (!flags_to_fmode(flags)) {
        res = -EINVAL;
        goto fail0;
    }
    if (S_ISDIR(inode->i_mode) && (flags & O_ACCMODE) != O_RDONLY) {
        res = -EISDIR;
        goto fail0;
    }
    if ((flags & O_DIRECTORY) && !S_ISDIR(inode->i_mode)) {
        res = -ENOTDIR;
        goto fail0;
    }
    if (!inode->i_fop) {
        res = -ENXIO;
        goto fail0;
    }

    fp = kmem_cache_alloc(file_cache, M_SLEEPOK);
    if (!fp) {
        res = -ENOMEM;
        goto fail0;
    }

    memset(fp, 0, sizeof(struct file));
    fp->f_path  = *p;
    fp->f_inode = inode;
    fp->f_op    = inode->i_fop;
    fp->f_flags = flags & ~(O_CREAT | O_EXCL | O_TRUNC | O_NOFOLLOW | O_CLOEXEC | O_CLOFORK);
    fp->f_mode  = flags_to_fmode(flags);
    mutex_init(&fp->f_pos_lock);
    refcount_init(&fp->f_refs, 1);

    if (fp->f_op->fop_open && (res = fp->f_op->fop_open(inode, fp))) { goto fail1; }

    if ((flags & O_TRUNC) && S_ISREG(inode->i_mode) && (fp->f_mode & FMODE_WRITE)) {
        if ((res = vfs_truncate(inode, 0))) {
            fput(fp);
            return res;
        }
    }

    *out = fp;
    return 0;

fail1:
    kmem_cache_free(file_cache, fp);
fail0:
    path_put(p);
    return res;
}

void fhold(struct file *fp) {
    BUG_ON(refcount_get(&fp->f_refs) == 0);
    refcount_inc(&fp->f_refs);
}

void fput(struct file *fp) {
    if (!refcount_dec_and_test(&fp->f_refs)) { return; }

    if (fp->f_op->fop_release) { (void) fp->f_op->fop_release(fp->f_inode, fp); }
    path_put(&fp->f_path);
    kmem_cache_free(file_cache, fp);
}

static int check_read(struct file *fp) {
    if (!(fp->f_mode & FMODE_READ)) { return -EBADF; }
    if (S_ISDIR(fp->f_inode->i_mode)) { return -EISDIR; }
    if (!fp->f_op->fop_read) { return -EINVAL; }
    return 0;
}

static int check_write(struct file *fp) {
    if (!(fp->f_mode & FMODE_WRITE)) { return -EBADF; }
    if (!fp->f_op->fop_write) { return -EINVAL; }
    return 0;
}

ssize_t vfs_read(struct file *fp, void *buf, size_t cnt) {
    ssize_t res;
    off_t   pos;

    if ((res = check_read(fp))) { return res; }

    mutex_lock(&fp->f_pos_lock);
    pos = fp->f_pos;
    res = fp->f_op->fop_read(fp, buf, cnt, &pos);
    if (res >= 0) { fp->f_pos = pos; }
    mutex_unlock(&fp->f_pos_lock);

    return res;
}

ssize_t vfs_write(struct file *fp, const void *buf, size_t cnt) {
    ssize_t res;
    off_t   pos;

    if ((res = check_write(fp))) { return res; }

    mutex_lock(&fp->f_pos_lock);
    pos = (fp->f_flags & O_APPEND) ? fp->f_inode->i_size : fp->f_pos;
    res = fp->f_op->fop_write(fp, buf, cnt, &pos);
    if (res >= 0) { fp->f_pos = pos; }
    mutex_unlock(&fp->f_pos_lock);

    return res;
}

ssize_t vfs_pread(struct file *fp, void *buf, size_t cnt, off_t off) {
    ssize_t res;

    if ((res = check_read(fp))) { return res; }
    if (off < 0) { return -EINVAL; }

    return fp->f_op->fop_read(fp, buf, cnt, &off);
}

ssize_t vfs_pwrite(struct file *fp, const void *buf, size_t cnt, off_t off) {
    ssize_t res;

    if ((res = check_write(fp))) { return res; }
    if (off < 0) { return -EINVAL; }

    return fp->f_op->fop_write(fp, buf, cnt, &off);
}

off_t generic_file_llseek(struct file *fp, off_t off, int whence) {
    off_t base;

    mutex_lock(&fp->f_pos_lock);
    switch (whence) {
        case SEEK_SET:
            base = 0;
            break;
        case SEEK_CUR:
            base = fp->f_pos;
            break;
        case SEEK_END:
            base = fp->f_inode->i_size;
            break;
        default:
            mutex_unlock(&fp->f_pos_lock);
            return -EINVAL;
    }

    if ((off > 0 && base > OFF_MAX - off) || base + off < 0) {
        mutex_unlock(&fp->f_pos_lock);
        return -EINVAL;
    }

    fp->f_pos = base + off;
    off       = fp->f_pos;
    mutex_unlock(&fp->f_pos_lock);

    return off;
}

off_t vfs_llseek(struct file *fp, off_t off, int whence) {
    if (fp->f_op->fop_llseek) { return fp->f_op->fop_llseek(fp, off, whence); }
    return generic_file_llseek(fp, off, whence);
}

int vfs_iterate(struct file *fp, struct dir_ctx *ctx) {
    struct inode *inode = fp->f_inode;
    int           res;

    if (!S_ISDIR(inode->i_mode)) { return -ENOTDIR; }
    if (!fp->f_op->fop_iterate) { return -EINVAL; }

    mutex_lock(&fp->f_pos_lock);
    mutex_lock(&inode->i_mutex);
    ctx->dc_cookie = (u64) fp->f_pos;
    res            = fp->f_op->fop_iterate(fp, ctx);
    fp->f_pos      = (off_t) ctx->dc_cookie;
    mutex_unlock(&inode->i_mutex);
    mutex_unlock(&fp->f_pos_lock);

    return res;
}
