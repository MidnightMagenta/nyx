#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/linkage.h>
#include <sys/minmax.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/mm_types.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>
#include <asi/page.h>

struct ramfs_sb_info {
    ino_t rs_next_ino;
};

struct ramfs_inode {
    char **ri_pages;
    size_t ri_npages;
    char  *ri_link;
};

#define RAMFS_I(inode) ((struct ramfs_inode *) (inode)->i_fs_private)
#define RAMFS_SB(sb)   ((struct ramfs_sb_info *) (sb)->sb_fs_private)

static const struct superops ramfs_sops;
static const struct inodeops ramfs_dir_iops;
static const struct inodeops ramfs_file_iops;
static const struct inodeops ramfs_link_iops;
static const struct fileops  ramfs_dir_fops;
static const struct fileops  ramfs_file_fops;

static struct inode *ramfs_new_inode(struct superblock *sb, mode_t mode, dev_t rdev) {
    struct inode       *inode = ialloc(sb);
    struct ramfs_inode *ri;

    if (!inode) { return NULL; }

    ri = kmalloc(sizeof(struct ramfs_inode), M_SLEEPOK);
    if (!ri) {
        i_clr_nlink(inode);
        iput(inode);
        return NULL;
    }
    memset(ri, 0, sizeof(struct ramfs_inode));

    inode->i_fs_private = ri;
    inode->i_ino        = __atomic_fetch_add(&RAMFS_SB(sb)->rs_next_ino, 1, ATOMIC_RELAXED);
    inode->i_mode       = mode;

    switch (mode & S_IFMT) {
        case S_IFREG:
            inode->i_op  = &ramfs_file_iops;
            inode->i_fop = &ramfs_file_fops;
            break;
        case S_IFDIR:
            inode->i_op  = &ramfs_dir_iops;
            inode->i_fop = &ramfs_dir_fops;
            i_set_nlink(inode, 2);
            break;
        case S_IFLNK:
            inode->i_op = &ramfs_link_iops;
            break;
        default:
            init_special_inode(inode, mode, rdev);
            break;
    }

    return inode;
}

static void ramfs_free_pages(struct ramfs_inode *ri, size_t from) {
    for (size_t i = from; i < ri->ri_npages; i++) {
        if (ri->ri_pages[i]) {
            kfree(ri->ri_pages[i]);
            ri->ri_pages[i] = NULL;
        }
    }
}

static void ramfs_evict_inode(struct inode *inode) {
    struct ramfs_inode *ri = RAMFS_I(inode);

    if (!ri) { return; }

    ramfs_free_pages(ri, 0);
    if (ri->ri_pages) { kfree(ri->ri_pages); }
    if (ri->ri_link) { kfree(ri->ri_link); }
    kfree(ri);
    inode->i_fs_private = NULL;
}

static void ramfs_put_super(struct superblock *sb) {
    kfree(sb->sb_fs_private);
}

static const struct superops ramfs_sops = {
        .sbo_evict_inode = ramfs_evict_inode,
        .sbo_put_super   = ramfs_put_super,
};

static void ramfs_attach(struct inode *dir, struct dentry *dent, struct inode *inode) {
    if (S_ISDIR(inode->i_mode)) { i_inc_nlink(dir); }

    dinstantiate(dent, inode);
    dget(dent);
}

static int ramfs_lookup(struct inode *dir, struct dentry *dent) {
    (void) dir;
    (void) dent;
    return 0;
}

static int ramfs_create(struct inode *dir, struct dentry *dent, mode_t mode, dev_t rdev) {
    struct inode *inode = ramfs_new_inode(dir->i_sb, mode, rdev);
    if (!inode) { return -ENOMEM; }

    ramfs_attach(dir, dent, inode);
    return 0;
}

static int ramfs_symlink(struct inode *dir, struct dentry *dent, const char *target) {
    size_t        len = strlen(target);
    struct inode *inode;
    char         *link;

    link = kmalloc(len + 1, M_SLEEPOK);
    if (!link) { return -ENOMEM; }
    memcpy(link, target, len + 1);

    inode = ramfs_new_inode(dir->i_sb, S_IFLNK | 0777, 0);
    if (!inode) {
        kfree(link);
        return -ENOMEM;
    }

    RAMFS_I(inode)->ri_link = link;
    inode->i_size           = (off_t) len;

    ramfs_attach(dir, dent, inode);
    return 0;
}

static bool ramfs_dir_empty(struct dentry *dent) {
    struct dentry *child;
    bool           empty = true;

    spin_lock(&dent->d_lock);
    list_for_each_entry(child, &dent->d_children, d_siblings) {
        if (child->d_inode) {
            empty = false;
            break;
        }
    }
    spin_unlock(&dent->d_lock);

    return empty;
}

static int ramfs_remove(struct inode *dir, struct dentry *dent) {
    struct inode *inode = dent->d_inode;

    if (S_ISDIR(inode->i_mode)) {
        if (!ramfs_dir_empty(dent)) { return -ENOTEMPTY; }
        i_drop_nlink(dir);
        i_clr_nlink(inode);
    } else {
        i_drop_nlink(inode);
    }

    dput(dent);
    return 0;
}

static ssize_t ramfs_readlink(struct dentry *dent, char *buf, size_t len) {
    struct inode *inode = dent->d_inode;
    size_t        n     = MIN((size_t) inode->i_size, len);

    memcpy(buf, RAMFS_I(inode)->ri_link, n);
    return (ssize_t) n;
}

static int ramfs_grow_pages(struct ramfs_inode *ri, size_t npages) {
    char **pages;
    size_t newn;

    if (npages <= ri->ri_npages) { return 0; }

    newn = MAX(npages, ri->ri_npages * 2);
    newn = MAX(newn, (size_t) 4);

    pages = kmalloc(sizeof(char *) * newn, M_SLEEPOK);
    if (!pages) { return -ENOMEM; }

    memset(pages, 0, sizeof(char *) * newn);
    if (ri->ri_pages) {
        memcpy(pages, ri->ri_pages, sizeof(char *) * ri->ri_npages);
        kfree(ri->ri_pages);
    }

    ri->ri_pages  = pages;
    ri->ri_npages = newn;
    return 0;
}

static int ramfs_truncate(struct inode *inode, off_t size) {
    struct ramfs_inode *ri   = RAMFS_I(inode);
    size_t              keep = PG_ALIGN_UP((size_t) size) / PAGE_SIZE;
    size_t              tail = (size_t) size % PAGE_SIZE;

    if (size < inode->i_size) {
        ramfs_free_pages(ri, keep);
        if (tail && keep - 1 < ri->ri_npages && ri->ri_pages[keep - 1]) {
            memset(ri->ri_pages[keep - 1] + tail, 0, PAGE_SIZE - tail);
        }
    }

    inode->i_size = size;
    return 0;
}

static ssize_t ramfs_read(struct file *fp, char *buf, size_t cnt, off_t *pos) {
    struct inode       *inode = fp->f_inode;
    struct ramfs_inode *ri    = RAMFS_I(inode);
    size_t              done  = 0;
    size_t              off, n;

    if (*pos < 0) { return -EINVAL; }

    mutex_lock(&inode->i_mutex);

    if (*pos >= inode->i_size) {
        mutex_unlock(&inode->i_mutex);
        return 0;
    }

    off = (size_t) *pos;
    n   = MIN(cnt, (size_t) inode->i_size - off);

    while (done < n) {
        size_t pg    = (off + done) / PAGE_SIZE;
        size_t pgoff = (off + done) % PAGE_SIZE;
        size_t chunk = MIN(n - done, PAGE_SIZE - pgoff);

        if (pg < ri->ri_npages && ri->ri_pages[pg]) {
            memcpy(buf + done, ri->ri_pages[pg] + pgoff, chunk);
        } else {
            memset(buf + done, 0, chunk);
        }
        done += chunk;
    }

    mutex_unlock(&inode->i_mutex);

    *pos += (off_t) done;
    return (ssize_t) done;
}

static ssize_t ramfs_write(struct file *fp, const char *buf, size_t cnt, off_t *pos) {
    struct inode       *inode = fp->f_inode;
    struct ramfs_inode *ri    = RAMFS_I(inode);
    size_t              done  = 0;
    size_t              off;
    int                 res = 0;

    if (*pos < 0) { return -EINVAL; }
    if (cnt > (size_t) (OFF_MAX - *pos)) { return -EFBIG; }

    off = (size_t) *pos;

    mutex_lock(&inode->i_mutex);

    if ((res = ramfs_grow_pages(ri, PG_ALIGN_UP(off + cnt) / PAGE_SIZE))) { goto out; }

    while (done < cnt) {
        size_t pg    = (off + done) / PAGE_SIZE;
        size_t pgoff = (off + done) % PAGE_SIZE;
        size_t chunk = MIN(cnt - done, PAGE_SIZE - pgoff);

        if (!ri->ri_pages[pg]) {
            ri->ri_pages[pg] = kmalloc(PAGE_SIZE, M_SLEEPOK);
            if (!ri->ri_pages[pg]) {
                res = -ENOMEM;
                break;
            }
            memset(ri->ri_pages[pg], 0, PAGE_SIZE);
        }

        memcpy(ri->ri_pages[pg] + pgoff, buf + done, chunk);
        done += chunk;
    }

    if ((off_t) (off + done) > inode->i_size) { inode->i_size = (off_t) (off + done); }

out:
    mutex_unlock(&inode->i_mutex);

    if (done == 0 && res) { return res; }

    *pos += (off_t) done;
    return (ssize_t) done;
}

static struct dentry *ramfs_next_positive(struct dentry *dir, struct list_head *from, size_t skip) {
    struct dentry *found = NULL;

    spin_lock(&dir->d_lock);
    for (struct list_head *pos = from->next; pos != &dir->d_children; pos = pos->next) {
        struct dentry *child = list_entry(pos, struct dentry, d_siblings);

        if (!child->d_inode) { continue; }
        if (skip) {
            skip--;
            continue;
        }

        refcount_inc(&child->d_refs);
        found = child;
        break;
    }
    spin_unlock(&dir->d_lock);

    return found;
}

static int ramfs_iterate(struct file *fp, struct dir_ctx *ctx) {
    struct dentry *dir    = fp->f_path.p_dent;
    u64            cookie = ctx->dc_cookie;
    struct dentry *child;

    if (cookie == 0) {
        if (ctx->dc_callback(ctx, ".", 1, 0, dir->d_inode->i_ino, DT_DIR)) { return 0; }
        ctx->dc_cookie = ++cookie;
    }
    if (cookie == 1) {
        if (ctx->dc_callback(ctx, "..", 2, 1, dir->d_parent->d_inode->i_ino, DT_DIR)) { return 0; }
        ctx->dc_cookie = ++cookie;
    }

    child = ramfs_next_positive(dir, &dir->d_children, (size_t) (cookie - 2));
    while (child) {
        struct inode  *inode = child->d_inode;
        struct dentry *next;

        if (ctx->dc_callback(ctx,
                             child->d_name,
                             (int) child->d_namelen,
                             (off_t) cookie,
                             inode->i_ino,
                             IFTODT(inode->i_mode))) {
            dput(child);
            break;
        }

        ctx->dc_cookie = ++cookie;
        next           = ramfs_next_positive(dir, &child->d_siblings, 0);
        dput(child);
        child = next;
    }

    return 0;
}

static const struct inodeops ramfs_dir_iops = {
        .iop_lookup  = ramfs_lookup,
        .iop_create  = ramfs_create,
        .iop_symlink = ramfs_symlink,
        .iop_remove  = ramfs_remove,
};

static const struct inodeops ramfs_file_iops = {
        .iop_truncate = ramfs_truncate,
};

static const struct inodeops ramfs_link_iops = {
        .iop_readlink = ramfs_readlink,
};

static const struct fileops ramfs_dir_fops = {
        .fop_iterate = ramfs_iterate,
};

static const struct fileops ramfs_file_fops = {
        .fop_read  = ramfs_read,
        .fop_write = ramfs_write,
};

static int ramfs_mount(struct fstype      *fs,
                       const char         *source,
                       unsigned long       flags,
                       const void         *data,
                       struct superblock **out) {
    struct superblock    *sb;
    struct ramfs_sb_info *info;
    struct inode         *root;

    (void) source;
    (void) flags;
    (void) data;

    sb = salloc(fs, &ramfs_sops);
    if (!sb) { return -ENOMEM; }

    info = kmalloc(sizeof(struct ramfs_sb_info), M_SLEEPOK);
    if (!info) {
        kfree(sb);
        return -ENOMEM;
    }

    info->rs_next_ino = 1;
    sb->sb_fs_private = info;
    sb->sb_block_size = PAGE_SIZE;

    root = ramfs_new_inode(sb, S_IFDIR | 0755, 0);
    if (!root) {
        kfree(info);
        kfree(sb);
        return -ENOMEM;
    }

    sb->sb_root = dalloc_root(sb, root);
    if (!sb->sb_root) {
        iput(root);
        kfree(info);
        kfree(sb);
        return -ENOMEM;
    }

    *out = sb;
    return 0;
}

static void ramfs_kill_sb(struct superblock *sb) {
    if (sb->sb_root) { dshrink(sb->sb_root, 1); }
    kill_generic_super(sb);
}

static struct fstype ramfs_type = {
        .fs_name    = "ramfs",
        .fs_mount   = ramfs_mount,
        .fs_kill_sb = ramfs_kill_sb,
};

void __init ramfs_init() {
    BUG_ON(vfs_register_fs(&ramfs_type));
}
