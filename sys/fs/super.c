#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/list.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/mm_types.h>
#include <sys/spinlock.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>

static LIST_HEAD(fstypes);
static spinlock_t fstypes_lock;

static struct fstype *__find_fs(const char *name) {
    struct fstype *fs;

    list_for_each_entry(fs, &fstypes, fs_linkage) {
        if (!strcmp(fs->fs_name, name)) { return fs; }
    }

    return NULL;
}

int vfs_register_fs(struct fstype *fs) {
    int res = 0;

    if (!fs->fs_name || !fs->fs_mount) { return -EINVAL; }

    spin_lock(&fstypes_lock);
    if (__find_fs(fs->fs_name)) {
        res = -EEXIST;
    } else {
        list_add_tail(&fs->fs_linkage, &fstypes);
    }
    spin_unlock(&fstypes_lock);

    return res;
}

int vfs_unregister_fs(struct fstype *fs) {
    int res = -ENOENT;

    spin_lock(&fstypes_lock);
    if (__find_fs(fs->fs_name) == fs) {
        list_del(&fs->fs_linkage);
        res = 0;
    }
    spin_unlock(&fstypes_lock);

    return res;
}

struct fstype *vfs_get_fs(const char *name) {
    struct fstype *fs;

    spin_lock(&fstypes_lock);
    fs = __find_fs(name);
    spin_unlock(&fstypes_lock);

    return fs;
}

struct superblock *salloc(struct fstype *fs, const struct superops *ops) {
    struct superblock *sb = kmalloc(sizeof(struct superblock), M_SLEEPOK);
    if (!sb) { return NULL; }

    memset(sb, 0, sizeof(struct superblock));
    sb->sb_type = fs;
    sb->sb_ops  = ops;
    refcount_init(&sb->sb_refs, 1);
    spinlock_init(&sb->sb_lock);
    mutex_init(&sb->sb_inode_mutex);
    list_init(&sb->sb_inodes);

    return sb;
}

void sget(struct superblock *sb) {
    BUG_ON(refcount_get(&sb->sb_refs) == 0);
    refcount_inc(&sb->sb_refs);
}

void sput(struct superblock *sb) {
    if (!refcount_dec_and_test(&sb->sb_refs)) { return; }

    if (sb->sb_ops && sb->sb_ops->sbo_sync_fs) { (void) sb->sb_ops->sbo_sync_fs(sb); }

    if (sb->sb_type->fs_kill_sb) {
        sb->sb_type->fs_kill_sb(sb);
    } else {
        kill_generic_super(sb);
    }
}

void kill_generic_super(struct superblock *sb) {
    struct dentry *root = sb->sb_root;

    sb->sb_root = NULL;
    if (root) { dput(root); }

    BUG_ON(!list_is_empty(&sb->sb_inodes));

    if (sb->sb_ops && sb->sb_ops->sbo_put_super) { sb->sb_ops->sbo_put_super(sb); }
    kfree(sb);
}
