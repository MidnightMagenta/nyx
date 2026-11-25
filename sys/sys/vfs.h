#ifndef _NYX_VFS_H
#define _NYX_VFS_H

#include <sys/limits.h>
#include <sys/list.h>
#include <sys/mutex.h>
#include <sys/posix_types.h>
#include <sys/refcount.h>
#include <sys/spinlock.h>
#include <sys/stddef.h>

#define MAXSYMLINKS 32

#define MINORBITS         20
#define MINORMASK         ((1UL << MINORBITS) - 1)
#define makedev(maj, min) ((dev_t) (((dev_t) (maj) << MINORBITS) | ((dev_t) (min) & MINORMASK)))
#define major(dev)        ((unsigned int) ((dev) >> MINORBITS))
#define minor(dev)        ((unsigned int) ((dev) & MINORMASK))

struct path;
struct dentry;
struct inode;
struct inodeops;
struct fileops;
struct mount;
struct superblock;
struct superops;
struct file;
struct dir_ctx;

struct path {
    struct mount  *p_mnt;
    struct dentry *p_dent;
};

struct fstype {
    const char      *fs_name;
    int              (*fs_mount)(struct fstype      *fs,
                                 const char         *source,
                                 unsigned long       flags,
                                 const void         *data,
                                 struct superblock **out);
    void             (*fs_kill_sb)(struct superblock *sb);
    struct list_head fs_linkage;
};

struct superops {
    struct inode *(*sbo_read_inode)(struct superblock *sb, ino_t ino);
    int           (*sbo_write_inode)(struct inode *ino);
    void          (*sbo_evict_inode)(struct inode *ino);
    int           (*sbo_sync_fs)(struct superblock *sb);
    // int        (*sbo_statfs)(struct superblock *sb, struct statfs *out); //: TODO: implement statfs
    void (*sbo_put_super)(struct superblock *sb);
};

/* Every operation that takes a directory inode is called with that directory's i_mutex held. */
struct inodeops {
    int     (*iop_lookup)(struct inode *dir, struct dentry *dent);
    int     (*iop_create)(struct inode *dir, struct dentry *dent, mode_t mode, dev_t rdev);
    int     (*iop_symlink)(struct inode *dir, struct dentry *dent, const char *target);
    int     (*iop_remove)(struct inode *dir, struct dentry *dent);
    ssize_t (*iop_readlink)(struct dentry *dent, char *buf, size_t len);
    int     (*iop_truncate)(struct inode *ino, off_t size);
};

typedef int (*filldir_t)(struct dir_ctx *ctx, const char *name, int namelen, off_t pos, u64 ino, unsigned int type);

struct dir_ctx {
    filldir_t dc_callback;
    u64       dc_cookie;
};

/* Buffers passed to fop_read and fop_write are always kernel memory. */
struct fileops {
    int     (*fop_open)(struct inode *in, struct file *fp);
    int     (*fop_release)(struct inode *in, struct file *fp);
    ssize_t (*fop_read)(struct file *fp, char *buf, size_t cnt, off_t *pos);
    ssize_t (*fop_write)(struct file *fp, const char *buf, size_t cnt, off_t *pos);
    off_t   (*fop_llseek)(struct file *fp, off_t off, int whence);
    int     (*fop_iterate)(struct file *fp, struct dir_ctx *ctx);
};

struct superblock {
    // TODO: backing block dev
    struct fstype         *sb_type;
    const struct superops *sb_ops;
    struct dentry         *sb_root;
    unsigned long          sb_block_size;
    unsigned long          sb_flags;
    refcount_t             sb_refs;
    void                  *sb_fs_private;
    spinlock_t             sb_lock;
    struct mutex           sb_inode_mutex;
    struct list_head       sb_inodes;
};

struct mount {
    struct dentry     *mnt_root;
    struct superblock *mnt_sb;
    struct mount      *mnt_parent;
    struct dentry     *mnt_mountpoint;
    refcount_t         mnt_refs;
    struct list_head   mnt_linkage;
};

#define D_DEAD (1 << 0)

struct dentry {
    char               d_name[NAME_MAX + 1];
    size_t             d_namelen;
    struct dentry     *d_parent;
    struct inode      *d_inode;
    struct superblock *d_sb;
    refcount_t         d_refs;
    unsigned long      d_flags;
    unsigned int       d_mounts;
    spinlock_t         d_lock;
    struct list_head   d_children;
    struct list_head   d_siblings;
    void              *d_fs_private;
};

struct inode {
    ino_t                  i_ino;
    mode_t                 i_mode;
    nlink_t                i_nlink;
    off_t                  i_size;
    dev_t                  i_rdev;
    refcount_t             i_refs;
    struct superblock     *i_sb;
    const struct inodeops *i_op;
    const struct fileops  *i_fop;
    spinlock_t             i_lock;
    struct mutex           i_mutex;
    struct list_head       i_sb_linkage;
    void                  *i_fs_private;
};

#define FMODE_READ  (1 << 0)
#define FMODE_WRITE (1 << 1)

struct file {
    struct path           f_path;
    struct inode         *f_inode;
    const struct fileops *f_op;
    off_t                 f_pos;
    struct mutex          f_pos_lock;
    unsigned long         f_flags;
    unsigned long         f_mode;
    refcount_t            f_refs;
    void                 *f_fs_private;
};

#define LOOKUP_FOLLOW    (1 << 0)
#define LOOKUP_DIRECTORY (1 << 1)

static inline struct inode *d_inode(struct dentry *dent) {
    return dent->d_inode;
}

static inline struct inode *f_inode(struct file *f) {
    return f->f_inode;
}

static inline bool d_is_negative(struct dentry *dent) {
    return dent->d_inode == NULL;
}

static inline void i_set_nlink(struct inode *inode, nlink_t nl) {
    inode->i_nlink = nl;
}
static inline void i_inc_nlink(struct inode *inode) {
    inode->i_nlink++;
}
static inline void i_drop_nlink(struct inode *inode) {
    inode->i_nlink--;
}
static inline void i_clr_nlink(struct inode *inode) {
    inode->i_nlink = 0;
}

int            vfs_register_fs(struct fstype *fs);
int            vfs_unregister_fs(struct fstype *fs);
struct fstype *vfs_get_fs(const char *name);

struct superblock *salloc(struct fstype *fs, const struct superops *ops);
void               sget(struct superblock *sb);
void               sput(struct superblock *sb);
void               kill_generic_super(struct superblock *sb);

struct dentry *dalloc(struct dentry *parent, const char *name, size_t namelen);
struct dentry *dalloc_root(struct superblock *sb, struct inode *root);
struct dentry *dlookup(struct dentry *parent, const char *name, size_t namelen);
void           dget(struct dentry *dent);
void           dput(struct dentry *dent);
void           dinstantiate(struct dentry *dent, struct inode *inode);
void           dunlink(struct dentry *dent);
void           dshrink(struct dentry *root, int held);

struct inode *ialloc(struct superblock *sb);
struct inode *iread(struct superblock *sb, ino_t ino);
void          iget(struct inode *inode);
void          iput(struct inode *inode);
void          init_special_inode(struct inode *inode, mode_t mode, dev_t rdev);

void          mntget(struct mount *mnt);
void          mntput(struct mount *mnt);
struct mount *lookup_mnt(struct mount *parent, struct dentry *mountpoint);

void path_get(struct path *p);
void path_put(struct path *p);

int vfs_mount_root(const char *fsname, const char *source, unsigned long flags, const void *data);
int vfs_mount(const char *fsname, const char *source, const char *target, unsigned long flags, const void *data);
int vfs_umount(const char *target);

int vfs_lookup(const struct path *dir, const char *name, unsigned int flags, struct path *out);
int vfs_mknod(const struct path *dir, const char *name, mode_t mode, dev_t rdev);
int vfs_mkdir(const struct path *dir, const char *name, mode_t mode);
int vfs_symlink(const struct path *dir, const char *name, const char *target);
int vfs_unlink(const struct path *dir, const char *name);
int vfs_rmdir(const struct path *dir, const char *name);

int     vfs_open(const struct path *dir, const char *name, int flags, mode_t mode, struct file **out);
void    fhold(struct file *fp);
void    fput(struct file *fp);
ssize_t vfs_read(struct file *fp, void *buf, size_t cnt);
ssize_t vfs_write(struct file *fp, const void *buf, size_t cnt);
ssize_t vfs_pread(struct file *fp, void *buf, size_t cnt, off_t off);
ssize_t vfs_pwrite(struct file *fp, const void *buf, size_t cnt, off_t off);
off_t   vfs_llseek(struct file *fp, off_t off, int whence);
int     vfs_iterate(struct file *fp, struct dir_ctx *ctx);
int     vfs_truncate(struct inode *inode, off_t size);

off_t generic_file_llseek(struct file *fp, off_t off, int whence);

int chrdev_register(dev_t dev, const char *name, const struct fileops *fops);
int chrdev_unregister(dev_t dev);

extern const struct fileops chrdev_fops;

#endif
