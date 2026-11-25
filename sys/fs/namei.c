#include <sys/current.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/mm_types.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#include "internal.h"

struct nameidata {
    struct path  nd_path;
    struct path  nd_root;
    char        *nd_buf;
    char        *nd_spare;
    const char  *nd_last;
    size_t       nd_lastlen;
    bool         nd_trailing;
    int          nd_links;
    unsigned int nd_flags;
};

int get_start_paths(const struct path *dir, const char *name, struct path *start, struct path *root) {
    struct filedesc *fdp = current()->t_proc->p_fd;

    if (!fdp) { return -ENOENT; }

    fd_get_root(fdp, root);
    if (!root->p_mnt) { return -ENOENT; }

    if (name[0] == '/') {
        *start = *root;
        path_get(start);
    } else if (dir) {
        *start = *dir;
        path_get(start);
    } else {
        fd_get_cwd(fdp, start);
        if (!start->p_mnt) {
            path_put(root);
            return -ENOENT;
        }
    }

    return 0;
}

static void nd_release(struct nameidata *nd) {
    if (nd->nd_path.p_mnt) { path_put(&nd->nd_path); }
    if (nd->nd_root.p_mnt) { path_put(&nd->nd_root); }
    kfree(nd->nd_buf);
    if (nd->nd_spare) { kfree(nd->nd_spare); }
}

static int nd_init(struct nameidata *nd, const struct path *dir, const char *name, unsigned int flags) {
    size_t len = strnlen(name, PATH_MAX);
    int    res;

    memset(nd, 0, sizeof(struct nameidata));
    nd->nd_flags = flags;

    if (len == 0) { return -ENOENT; }
    if (len >= PATH_MAX) { return -ENAMETOOLONG; }

    nd->nd_buf = kmalloc(PATH_MAX, M_SLEEPOK);
    if (!nd->nd_buf) { return -ENOMEM; }
    memcpy(nd->nd_buf, name, len + 1);

    if ((res = get_start_paths(dir, name, &nd->nd_path, &nd->nd_root))) {
        memset(&nd->nd_path, 0, sizeof(struct path));
        memset(&nd->nd_root, 0, sizeof(struct path));
        kfree(nd->nd_buf);
        return res;
    }

    return 0;
}

static struct dentry *__lookup_locked(struct dentry *parent, const char *name, size_t len, int *err) {
    struct inode  *dir = parent->d_inode;
    struct dentry *child;
    int            res;

    child = dlookup(parent, name, len);
    if (child) { return child; }

    child = dalloc(parent, name, len);
    if (!child) {
        *err = -ENOMEM;
        return NULL;
    }

    if (dir->i_op && dir->i_op->iop_lookup) {
        if ((res = dir->i_op->iop_lookup(dir, child))) {
            dput(child);
            *err = res;
            return NULL;
        }
    }

    return child;
}

static struct dentry *lookup_one(struct dentry *parent, const char *name, size_t len, int *err) {
    struct dentry *child = dlookup(parent, name, len);

    if (child && !d_is_negative(child)) { return child; }
    if (child) { dput(child); }

    mutex_lock(&parent->d_inode->i_mutex);
    child = __lookup_locked(parent, name, len, err);
    mutex_unlock(&parent->d_inode->i_mutex);

    return child;
}

static int nd_follow_link(struct nameidata *nd, struct path *link, const char *rest) {
    struct inode *inode   = link->p_dent->d_inode;
    size_t        restlen = strlen(rest);
    ssize_t       n;
    char         *tmp;

    if (++nd->nd_links > MAXSYMLINKS) {
        path_put(link);
        return -ELOOP;
    }
    if (!inode->i_op || !inode->i_op->iop_readlink) {
        path_put(link);
        return -EINVAL;
    }
    if (!nd->nd_spare && !(nd->nd_spare = kmalloc(PATH_MAX, M_SLEEPOK))) {
        path_put(link);
        return -ENOMEM;
    }

    n = inode->i_op->iop_readlink(link->p_dent, nd->nd_spare, PATH_MAX);
    path_put(link);
    if (n < 0) { return (int) n; }
    if (n == 0) { return -ENOENT; }
    if ((size_t) n + 1 + restlen >= PATH_MAX) { return -ENAMETOOLONG; }

    if (restlen) {
        nd->nd_spare[n] = '/';
        memcpy(nd->nd_spare + n + 1, rest, restlen + 1);
    } else {
        nd->nd_spare[n] = '\0';
    }

    tmp          = nd->nd_buf;
    nd->nd_buf   = nd->nd_spare;
    nd->nd_spare = tmp;

    if (nd->nd_buf[0] == '/') {
        path_put(&nd->nd_path);
        nd->nd_path = nd->nd_root;
        path_get(&nd->nd_path);
    }

    return 0;
}

static int nd_walk(struct nameidata *nd, bool want_parent) {
    char *cur = nd->nd_buf;

    for (;;) {
        const char    *comp;
        size_t         len;
        char          *next;
        bool           last, trailing;
        struct dentry *child;
        struct path    np;
        int            res = 0;

        while (*cur == '/') { cur++; }
        if (*cur == '\0') {
            nd->nd_last    = cur;
            nd->nd_lastlen = 0;
            return 0;
        }

        comp = cur;
        while (*cur && *cur != '/') { cur++; }
        len  = (size_t) (cur - comp);
        next = cur;
        while (*next == '/') { next++; }
        last     = (*next == '\0');
        trailing = last && next != cur;

        if (len > NAME_MAX) { return -ENAMETOOLONG; }
        if (!S_ISDIR(nd->nd_path.p_dent->d_inode->i_mode)) { return -ENOTDIR; }

        if (last) { nd->nd_trailing = trailing; }
        if (want_parent && last) {
            nd->nd_last    = comp;
            nd->nd_lastlen = len;
            return 0;
        }

        if (len == 1 && comp[0] == '.') {
            cur = next;
            continue;
        }
        if (len == 2 && comp[0] == '.' && comp[1] == '.') {
            follow_dotdot(&nd->nd_path, &nd->nd_root);
            cur = next;
            continue;
        }

        child = lookup_one(nd->nd_path.p_dent, comp, len, &res);
        if (!child) { return res; }
        if (d_is_negative(child)) {
            dput(child);
            return -ENOENT;
        }

        np.p_mnt  = nd->nd_path.p_mnt;
        np.p_dent = child;
        mntget(np.p_mnt);
        follow_mounts(&np);

        if (S_ISLNK(np.p_dent->d_inode->i_mode) && (!last || trailing || (nd->nd_flags & LOOKUP_FOLLOW))) {
            if ((res = nd_follow_link(nd, &np, next))) { return res; }
            cur = nd->nd_buf;
            continue;
        }

        path_put(&nd->nd_path);
        nd->nd_path = np;
        cur         = next;
    }
}

static int nd_walk_parent(struct nameidata *nd, const struct path *dir, const char *name) {
    int res;

    if ((res = nd_init(nd, dir, name, 0))) { return res; }
    if ((res = nd_walk(nd, true))) { goto fail; }
    if (!S_ISDIR(nd->nd_path.p_dent->d_inode->i_mode)) {
        res = -ENOTDIR;
        goto fail;
    }

    return 0;

fail:
    nd_release(nd);
    return res;
}

static inline bool is_dot_or_dotdot(const char *name, size_t len) {
    return (len == 1 && name[0] == '.') || (len == 2 && name[0] == '.' && name[1] == '.');
}

int vfs_lookup(const struct path *dir, const char *name, unsigned int flags, struct path *out) {
    struct nameidata nd;
    int              res;

    if ((res = nd_init(&nd, dir, name, flags))) { return res; }
    if ((res = nd_walk(&nd, false))) { goto out; }

    if ((flags & LOOKUP_DIRECTORY || nd.nd_trailing) && !S_ISDIR(nd.nd_path.p_dent->d_inode->i_mode)) {
        res = -ENOTDIR;
        goto out;
    }

    *out = nd.nd_path;
    memset(&nd.nd_path, 0, sizeof(struct path));

out:
    nd_release(&nd);
    return res;
}

static int validate_create(struct nameidata *nd, struct inode **dirp) {
    struct inode *dir = nd->nd_path.p_dent->d_inode;

    if (nd->nd_lastlen == 0 || is_dot_or_dotdot(nd->nd_last, nd->nd_lastlen)) { return -EEXIST; }
    if (!dir->i_op) { return -EPERM; }

    *dirp = dir;
    return 0;
}

static struct dentry *lookup_create(struct nameidata *nd, int *err) {
    struct dentry *parent = nd->nd_path.p_dent;
    struct dentry *child;

    if (parent->d_flags & D_DEAD) {
        *err = -ENOENT;
        return NULL;
    }

    child = __lookup_locked(parent, nd->nd_last, nd->nd_lastlen, err);
    if (!child) { return NULL; }

    if (!d_is_negative(child)) {
        dput(child);
        *err = -EEXIST;
        return NULL;
    }

    return child;
}

int vfs_mknod(const struct path *dir, const char *name, mode_t mode, dev_t rdev) {
    struct nameidata nd;
    struct inode    *dinode;
    struct dentry   *child;
    int              res;

    if ((mode & S_IFMT) == 0) { mode |= S_IFREG; }
    switch (mode & S_IFMT) {
        case S_IFREG:
        case S_IFDIR:
        case S_IFCHR:
        case S_IFBLK:
        case S_IFIFO:
        case S_IFSOCK:
            break;
        default:
            return -EINVAL;
    }

    if ((res = nd_walk_parent(&nd, dir, name))) { return res; }
    if ((res = validate_create(&nd, &dinode))) { goto out; }
    if (!dinode->i_op->iop_create) {
        res = -EPERM;
        goto out;
    }
    if (nd.nd_trailing && !S_ISDIR(mode)) {
        res = -ENOTDIR;
        goto out;
    }

    mutex_lock(&dinode->i_mutex);
    child = lookup_create(&nd, &res);
    if (child) {
        res = dinode->i_op->iop_create(dinode, child, mode, rdev);
        dput(child);
    }
    mutex_unlock(&dinode->i_mutex);

out:
    nd_release(&nd);
    return res;
}

int vfs_mkdir(const struct path *dir, const char *name, mode_t mode) {
    return vfs_mknod(dir, name, S_IFDIR | (mode & ~S_IFMT), 0);
}

int vfs_symlink(const struct path *dir, const char *name, const char *target) {
    struct nameidata nd;
    struct inode    *dinode;
    struct dentry   *child;
    size_t           tlen = strnlen(target, PATH_MAX);
    int              res;

    if (tlen == 0) { return -ENOENT; }
    if (tlen == PATH_MAX) { return -ENAMETOOLONG; }

    if ((res = nd_walk_parent(&nd, dir, name))) { return res; }
    if ((res = validate_create(&nd, &dinode))) { goto out; }
    if (!dinode->i_op->iop_symlink) {
        res = -EPERM;
        goto out;
    }

    mutex_lock(&dinode->i_mutex);
    child = lookup_create(&nd, &res);
    if (child) {
        res = dinode->i_op->iop_symlink(dinode, child, target);
        dput(child);
    }
    mutex_unlock(&dinode->i_mutex);

out:
    nd_release(&nd);
    return res;
}

static int do_remove(const struct path *dir, const char *name, bool is_dir) {
    struct nameidata nd;
    struct inode    *dinode, *victim;
    struct dentry   *child;
    int              res;

    if ((res = nd_walk_parent(&nd, dir, name))) { return res; }

    dinode = nd.nd_path.p_dent->d_inode;
    if (nd.nd_lastlen == 0) {
        res = -EBUSY;
        goto out;
    }
    if (is_dot_or_dotdot(nd.nd_last, nd.nd_lastlen)) {
        res = is_dir ? -EINVAL : -EISDIR;
        goto out;
    }
    if (!dinode->i_op || !dinode->i_op->iop_remove) {
        res = -EPERM;
        goto out;
    }

    mutex_lock(&dinode->i_mutex);

    child = __lookup_locked(nd.nd_path.p_dent, nd.nd_last, nd.nd_lastlen, &res);
    if (!child) { goto unlock; }

    victim = child->d_inode;
    if (!victim) {
        res = -ENOENT;
    } else if (is_dir && !S_ISDIR(victim->i_mode)) {
        res = -ENOTDIR;
    } else if (!is_dir && S_ISDIR(victim->i_mode)) {
        res = -EISDIR;
    } else if (!is_dir && nd.nd_trailing) {
        res = -ENOTDIR;
    } else if (__atomic_load_n(&child->d_mounts, ATOMIC_ACQUIRE)) {
        res = -EBUSY;
    } else {
        if (is_dir) { mutex_lock(&victim->i_mutex); }
        res = dinode->i_op->iop_remove(dinode, child);
        if (!res) { dunlink(child); }
        if (is_dir) { mutex_unlock(&victim->i_mutex); }
    }
    dput(child);

unlock:
    mutex_unlock(&dinode->i_mutex);
out:
    nd_release(&nd);
    return res;
}

int vfs_unlink(const struct path *dir, const char *name) {
    return do_remove(dir, name, false);
}

int vfs_rmdir(const struct path *dir, const char *name) {
    return do_remove(dir, name, true);
}

static int open_create(const struct path *dir, const char *name, int flags, mode_t mode, struct path *out) {
    struct nameidata nd;
    struct inode    *dinode;
    struct dentry   *child;
    int              res;

    if ((res = nd_walk_parent(&nd, dir, name))) { return res; }
    if (nd.nd_lastlen == 0 || is_dot_or_dotdot(nd.nd_last, nd.nd_lastlen) || nd.nd_trailing) {
        res = -EISDIR;
        goto out;
    }

    dinode = nd.nd_path.p_dent->d_inode;
    mutex_lock(&dinode->i_mutex);

    if (nd.nd_path.p_dent->d_flags & D_DEAD) {
        res = -ENOENT;
        goto unlock;
    }

    child = __lookup_locked(nd.nd_path.p_dent, nd.nd_last, nd.nd_lastlen, &res);
    if (!child) { goto unlock; }

    if (d_is_negative(child)) {
        if (!dinode->i_op || !dinode->i_op->iop_create) {
            res = -EPERM;
        } else {
            res = dinode->i_op->iop_create(dinode, child, S_IFREG | (mode & ~S_IFMT), 0);
        }
        if (res) {
            dput(child);
            goto unlock;
        }
    } else if (flags & O_EXCL) {
        dput(child);
        res = -EEXIST;
        goto unlock;
    } else if (S_ISDIR(child->d_inode->i_mode)) {
        dput(child);
        res = -EISDIR;
        goto unlock;
    } else if (S_ISLNK(child->d_inode->i_mode)) {
        dput(child);
        res = -EAGAIN;
        goto unlock;
    }
    mutex_unlock(&dinode->i_mutex);

    out->p_mnt  = nd.nd_path.p_mnt;
    out->p_dent = child;
    mntget(out->p_mnt);
    follow_mounts(out);

    nd_release(&nd);
    return 0;

unlock:
    mutex_unlock(&dinode->i_mutex);
out:
    nd_release(&nd);
    return res;
}

int vfs_open(const struct path *dir, const char *name, int flags, mode_t mode, struct file **out) {
    struct path  p;
    unsigned int lflags = 0;
    int          res    = -EAGAIN;

    if (flags & O_CREAT) {
        res = open_create(dir, name, flags, mode, &p);
        if (res == -EAGAIN && (flags & O_NOFOLLOW)) { return -ELOOP; }
    }

    if (res == -EAGAIN) {
        if (!(flags & O_NOFOLLOW)) { lflags |= LOOKUP_FOLLOW; }
        if (flags & O_DIRECTORY) { lflags |= LOOKUP_DIRECTORY; }
        res = vfs_lookup(dir, name, lflags, &p);
    }
    if (res) { return res; }

    return vfs_open_path(&p, flags, out);
}
