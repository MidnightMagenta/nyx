#include <sys/current.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/list.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/mm_types.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#include "internal.h"

spinlock_t mount_lock;
static LIST_HEAD(mounts);

void mntget(struct mount *mnt) {
    BUG_ON(refcount_get(&mnt->mnt_refs) == 0);
    refcount_inc(&mnt->mnt_refs);
}

void mntput(struct mount *mnt) {
    if (!refcount_dec_and_test(&mnt->mnt_refs)) { return; }

    dput(mnt->mnt_root);
    sput(mnt->mnt_sb);
    kfree(mnt);
}

void path_get(struct path *p) {
    mntget(p->p_mnt);
    dget(p->p_dent);
}

void path_put(struct path *p) {
    dput(p->p_dent);
    mntput(p->p_mnt);
}

struct mount *lookup_mnt(struct mount *parent, struct dentry *mountpoint) {
    struct mount *mnt;
    struct mount *found = NULL;

    spin_lock(&mount_lock);
    list_for_each_entry(mnt, &mounts, mnt_linkage) {
        if (mnt->mnt_parent == parent && mnt->mnt_mountpoint == mountpoint && mnt != parent) {
            mntget(mnt);
            found = mnt;
            break;
        }
    }
    spin_unlock(&mount_lock);

    return found;
}

void follow_mounts(struct path *p) {
    while (__atomic_load_n(&p->p_dent->d_mounts, ATOMIC_ACQUIRE)) {
        struct mount *mnt = lookup_mnt(p->p_mnt, p->p_dent);
        if (!mnt) { return; }

        path_put(p);
        p->p_mnt  = mnt;
        p->p_dent = mnt->mnt_root;
        dget(p->p_dent);
    }
}

void follow_dotdot(struct path *p, const struct path *root) {
    for (;;) {
        struct mount  *parent;
        struct dentry *mountpoint;

        if (p->p_dent == root->p_dent && p->p_mnt == root->p_mnt) { return; }

        if (p->p_dent != p->p_mnt->mnt_root) {
            struct dentry *dent = p->p_dent->d_parent;

            dget(dent);
            dput(p->p_dent);
            p->p_dent = dent;
            break;
        }

        spin_lock(&mount_lock);
        parent     = p->p_mnt->mnt_parent;
        mountpoint = p->p_mnt->mnt_mountpoint;
        if (parent == p->p_mnt) {
            spin_unlock(&mount_lock);
            return;
        }
        mntget(parent);
        dget(mountpoint);
        spin_unlock(&mount_lock);

        path_put(p);
        p->p_mnt  = parent;
        p->p_dent = mountpoint;
    }

    follow_mounts(p);
}

static int do_fs_mount(const char    *fsname,
                       const char    *source,
                       unsigned long  flags,
                       const void    *data,
                       struct mount **out) {
    struct fstype     *fs = vfs_get_fs(fsname);
    struct superblock *sb;
    struct mount      *mnt;
    int                res;

    if (!fs) { return -ENODEV; }

    mnt = kmalloc(sizeof(struct mount), M_SLEEPOK);
    if (!mnt) { return -ENOMEM; }

    if ((res = fs->fs_mount(fs, source, flags, data, &sb))) {
        kfree(mnt);
        return res;
    }
    BUG_ON(!sb->sb_root);

    memset(mnt, 0, sizeof(struct mount));
    dget(sb->sb_root);
    mnt->mnt_root = sb->sb_root;
    mnt->mnt_sb   = sb;
    refcount_init(&mnt->mnt_refs, 1);
    list_init(&mnt->mnt_linkage);

    *out = mnt;
    return 0;
}

int vfs_mount_root(const char *fsname, const char *source, unsigned long flags, const void *data) {
    struct process *p = current()->t_proc;
    struct mount   *mnt;
    struct path     root;
    int             res;

    if ((res = do_fs_mount(fsname, source, flags, data, &mnt))) { return res; }

    mnt->mnt_parent     = mnt;
    mnt->mnt_mountpoint = mnt->mnt_root;

    spin_lock(&mount_lock);
    list_add_tail(&mnt->mnt_linkage, &mounts);
    spin_unlock(&mount_lock);

    root.p_mnt  = mnt;
    root.p_dent = mnt->mnt_root;
    fd_set_root(p->p_fd, &root);
    fd_set_cwd(p->p_fd, &root);

    return 0;
}

int vfs_mount(const char *fsname, const char *source, const char *target, unsigned long flags, const void *data) {
    struct path   mp;
    struct mount *mnt;
    int           res;

    if ((res = vfs_lookup(NULL, target, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &mp))) { return res; }

    if ((res = do_fs_mount(fsname, source, flags, data, &mnt))) {
        path_put(&mp);
        return res;
    }

    mutex_lock(&mp.p_dent->d_parent->d_inode->i_mutex);
    if (mp.p_dent->d_flags & D_DEAD) {
        mutex_unlock(&mp.p_dent->d_parent->d_inode->i_mutex);
        mntput(mnt);
        path_put(&mp);
        return -ENOENT;
    }

    mnt->mnt_parent     = mp.p_mnt;
    mnt->mnt_mountpoint = mp.p_dent;

    spin_lock(&mount_lock);
    list_add_tail(&mnt->mnt_linkage, &mounts);
    __atomic_add_fetch(&mp.p_dent->d_mounts, 1, ATOMIC_RELEASE);
    spin_unlock(&mount_lock);
    mutex_unlock(&mp.p_dent->d_parent->d_inode->i_mutex);

    return 0;
}

int vfs_umount(const char *target) {
    struct path   p;
    struct mount *mnt;
    int           res;

    if ((res = vfs_lookup(NULL, target, LOOKUP_FOLLOW, &p))) { return res; }

    mnt = p.p_mnt;
    if (p.p_dent != mnt->mnt_root) {
        path_put(&p);
        return -EINVAL;
    }

    spin_lock(&mount_lock);
    if (mnt->mnt_parent == mnt || refcount_get(&mnt->mnt_refs) != 2) {
        spin_unlock(&mount_lock);
        path_put(&p);
        return -EBUSY;
    }
    list_del(&mnt->mnt_linkage);
    __atomic_sub_fetch(&mnt->mnt_mountpoint->d_mounts, 1, ATOMIC_RELEASE);
    spin_unlock(&mount_lock);

    dput(mnt->mnt_mountpoint);
    mntput(mnt->mnt_parent);
    mnt->mnt_parent     = mnt;
    mnt->mnt_mountpoint = NULL;

    path_put(&p);
    mntput(mnt);

    return 0;
}
