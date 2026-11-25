#include <sys/errno.h>
#include <sys/list.h>
#include <sys/mm/mm_types.h>
#include <sys/mm/slab.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#include "internal.h"

static kmem_cache_t *inode_cache;

void inode_init() {
    inode_cache = kmem_create_cache("inode", sizeof(struct inode), _Alignof(struct inode), NULL, NULL, M_SLEEPOK);
    BUG_ON(!inode_cache);
}

struct inode *ialloc(struct superblock *sb) {
    struct inode *inode = kmem_cache_alloc(inode_cache, M_SLEEPOK);
    if (!inode) { return NULL; }

    memset(inode, 0, sizeof(struct inode));
    inode->i_sb    = sb;
    inode->i_nlink = 1;
    refcount_init(&inode->i_refs, 1);
    spinlock_init(&inode->i_lock);
    mutex_init(&inode->i_mutex);
    list_init(&inode->i_sb_linkage);

    spin_lock(&sb->sb_lock);
    list_add_tail(&inode->i_sb_linkage, &sb->sb_inodes);
    spin_unlock(&sb->sb_lock);

    return inode;
}

struct inode *iread(struct superblock *sb, ino_t ino) {
    struct inode *inode;
    struct inode *found = NULL;

    mutex_lock(&sb->sb_inode_mutex);

    spin_lock(&sb->sb_lock);
    list_for_each_entry(inode, &sb->sb_inodes, i_sb_linkage) {
        if (inode->i_ino == ino) {
            refcount_inc(&inode->i_refs);
            found = inode;
            break;
        }
    }
    spin_unlock(&sb->sb_lock);

    if (!found && sb->sb_ops && sb->sb_ops->sbo_read_inode) { found = sb->sb_ops->sbo_read_inode(sb, ino); }

    mutex_unlock(&sb->sb_inode_mutex);

    return found;
}

void iget(struct inode *inode) {
    BUG_ON(refcount_get(&inode->i_refs) == 0);
    refcount_inc(&inode->i_refs);
}

void iput(struct inode *inode) {
    struct superblock     *sb  = inode->i_sb;
    const struct superops *ops = sb->sb_ops;

    spin_lock(&sb->sb_lock);
    if (!refcount_dec_and_test(&inode->i_refs)) {
        spin_unlock(&sb->sb_lock);
        return;
    }
    list_del(&inode->i_sb_linkage);
    spin_unlock(&sb->sb_lock);

    if (ops && inode->i_nlink > 0 && ops->sbo_write_inode) { (void) ops->sbo_write_inode(inode); }
    if (ops && ops->sbo_evict_inode) { ops->sbo_evict_inode(inode); }

    kmem_cache_free(inode_cache, inode);
}

void init_special_inode(struct inode *inode, mode_t mode, dev_t rdev) {
    inode->i_mode = mode;
    inode->i_rdev = rdev;
    inode->i_fop  = S_ISCHR(mode) ? &chrdev_fops : NULL;
}

int vfs_truncate(struct inode *inode, off_t size) {
    int res;

    if (size < 0) { return -EINVAL; }
    if (S_ISDIR(inode->i_mode)) { return -EISDIR; }
    if (!S_ISREG(inode->i_mode)) { return -EINVAL; }
    if (!inode->i_op || !inode->i_op->iop_truncate) { return -EPERM; }

    mutex_lock(&inode->i_mutex);
    res = inode->i_op->iop_truncate(inode, size);
    mutex_unlock(&inode->i_mutex);

    return res;
}
