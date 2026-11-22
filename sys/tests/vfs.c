#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/testing.h>
#include <sys/vfs.h>

static int write_str(const char *path, const char *s, int flags) {
    struct file *fp;
    ssize_t      n;
    int          res;

    if ((res = vfs_open(NULL, path, O_WRONLY | O_CREAT | flags, 0644, &fp))) { return res; }
    n = vfs_write(fp, s, strlen(s));
    fput(fp);

    return n == (ssize_t) strlen(s) ? 0 : -EIO;
}

static ssize_t read_str(const char *path, char *buf, size_t len) {
    struct file *fp;
    ssize_t      n;
    int          res;

    if ((res = vfs_open(NULL, path, O_RDONLY, 0, &fp))) { return res; }
    n = vfs_read(fp, buf, len - 1);
    fput(fp);

    if (n >= 0) { buf[n] = '\0'; }
    return n;
}

KERNEL_TEST(vfs_initramfs_unpacked) {
    struct path p;

    EXPECT_EQ(vfs_lookup(NULL, "/sbin/init", LOOKUP_FOLLOW, &p), 0);
    EXPECT_TRUE(S_ISREG(d_inode(p.p_dent)->i_mode));
    EXPECT_TRUE(d_inode(p.p_dent)->i_size > 0);
    path_put(&p);
}

KERNEL_TEST(vfs_file_read_write) {
    char         buf[32];
    struct file *fp;

    EXPECT_EQ(vfs_mkdir(NULL, "/vfstest", 0755), 0);
    EXPECT_EQ(vfs_mkdir(NULL, "/vfstest", 0755), -EEXIST);
    EXPECT_EQ(write_str("/vfstest/a", "hello", O_EXCL), 0);
    EXPECT_EQ(write_str("/vfstest/a", "x", O_EXCL), -EEXIST);
    EXPECT_EQ(read_str("/vfstest/a", buf, sizeof(buf)), 5);
    EXPECT_EQ(strcmp(buf, "hello"), 0);

    EXPECT_EQ(write_str("/vfstest/a", " world", O_APPEND), 0);
    EXPECT_EQ(read_str("/vfstest/./../vfstest//a", buf, sizeof(buf)), 11);
    EXPECT_EQ(strcmp(buf, "hello world"), 0);

    EXPECT_EQ(vfs_open(NULL, "/vfstest/a", O_RDWR, 0, &fp), 0);
    EXPECT_EQ(vfs_llseek(fp, 6, SEEK_SET), 6);
    EXPECT_EQ(vfs_read(fp, buf, 5), 5);
    EXPECT_EQ(memcmp(buf, "world", 5), 0);
    EXPECT_EQ(vfs_llseek(fp, -5, SEEK_END), 6);
    EXPECT_EQ(vfs_pwrite(fp, "W", 1, 6), 1);
    EXPECT_EQ(vfs_pread(fp, buf, 5, 6), 5);
    EXPECT_EQ(memcmp(buf, "World", 5), 0);
    fput(fp);

    EXPECT_EQ(write_str("/vfstest/a", "hi", O_TRUNC), 0);
    EXPECT_EQ(read_str("/vfstest/a", buf, sizeof(buf)), 2);
    EXPECT_EQ(vfs_open(NULL, "/vfstest/a/", O_RDONLY, 0, &fp), -ENOTDIR);
    EXPECT_EQ(vfs_open(NULL, "/vfstest", O_WRONLY, 0, &fp), -EISDIR);
    EXPECT_EQ(vfs_open(NULL, "/vfstest/missing", O_RDONLY, 0, &fp), -ENOENT);
}

KERNEL_TEST(vfs_large_file) {
    static char  pattern[3 * 4096 + 123];
    static char  back[sizeof(pattern)];
    struct file *fp;

    for (size_t i = 0; i < sizeof(pattern); i++) { pattern[i] = (char) (i * 7); }

    EXPECT_EQ(vfs_open(NULL, "/vfstest/big", O_RDWR | O_CREAT, 0644, &fp), 0);
    EXPECT_EQ(vfs_pwrite(fp, pattern, sizeof(pattern), 0), (ssize_t) sizeof(pattern));
    EXPECT_EQ(vfs_pread(fp, back, sizeof(back), 0), (ssize_t) sizeof(back));
    EXPECT_EQ(memcmp(pattern, back, sizeof(pattern)), 0);

    EXPECT_EQ(vfs_truncate(f_inode(fp), 5000), 0);
    EXPECT_EQ(vfs_truncate(f_inode(fp), 9000), 0);
    EXPECT_EQ(vfs_pread(fp, back, sizeof(back), 0), 9000);
    EXPECT_EQ(memcmp(pattern, back, 5000), 0);
    EXPECT_EQ(back[5000], 0);
    EXPECT_EQ(back[8999], 0);
    fput(fp);
}

KERNEL_TEST(vfs_symlinks) {
    char        buf[32];
    struct path p;

    EXPECT_EQ(vfs_mkdir(NULL, "/vfstest/d", 0755), 0);
    EXPECT_EQ(write_str("/vfstest/d/f", "linked", 0), 0);
    EXPECT_EQ(vfs_symlink(NULL, "/vfstest/rel", "d/f"), 0);
    EXPECT_EQ(vfs_symlink(NULL, "/vfstest/abs", "/vfstest/d"), 0);
    EXPECT_EQ(vfs_symlink(NULL, "/vfstest/loop", "/vfstest/loop"), 0);

    EXPECT_EQ(read_str("/vfstest/rel", buf, sizeof(buf)), 6);
    EXPECT_EQ(strcmp(buf, "linked"), 0);
    EXPECT_EQ(read_str("/vfstest/abs/f", buf, sizeof(buf)), 6);
    EXPECT_EQ(read_str("/vfstest/abs/../a", buf, sizeof(buf)), 2);
    EXPECT_EQ(read_str("/vfstest/loop", buf, sizeof(buf)), -ELOOP);

    EXPECT_EQ(vfs_lookup(NULL, "/vfstest/rel", 0, &p), 0);
    EXPECT_TRUE(S_ISLNK(d_inode(p.p_dent)->i_mode));
    path_put(&p);
}

struct listing {
    struct dir_ctx ctx;
    int            count;
    bool           saw_f;
};

static int count_entry(struct dir_ctx *ctx, const char *name, int namelen, off_t pos, u64 ino, unsigned int type) {
    struct listing *l = (struct listing *) ctx;

    (void) pos;
    (void) ino;

    l->count++;
    if (namelen == 1 && name[0] == 'f' && type == DT_REG) { l->saw_f = true; }
    return 0;
}

KERNEL_TEST(vfs_readdir) {
    struct listing l = {.ctx = {.dc_callback = count_entry}};
    struct file   *fp;

    EXPECT_EQ(vfs_open(NULL, "/vfstest/d", O_RDONLY | O_DIRECTORY, 0, &fp), 0);
    EXPECT_EQ(vfs_iterate(fp, &l.ctx), 0);
    EXPECT_EQ(l.count, 3);
    EXPECT_TRUE(l.saw_f);

    l.count = 0;
    EXPECT_EQ(vfs_iterate(fp, &l.ctx), 0);
    EXPECT_EQ(l.count, 0);
    fput(fp);
}

KERNEL_TEST(vfs_remove) {
    char         buf[16];
    struct file *fp;

    EXPECT_EQ(vfs_rmdir(NULL, "/vfstest/d"), -ENOTEMPTY);
    EXPECT_EQ(vfs_unlink(NULL, "/vfstest/d"), -EISDIR);
    EXPECT_EQ(vfs_rmdir(NULL, "/vfstest/d/f"), -ENOTDIR);

    EXPECT_EQ(vfs_open(NULL, "/vfstest/d/f", O_RDONLY, 0, &fp), 0);
    EXPECT_EQ(vfs_unlink(NULL, "/vfstest/d/f"), 0);
    EXPECT_EQ(vfs_open(NULL, "/vfstest/d/f", O_RDONLY, 0, &fp), -ENOENT);
    EXPECT_EQ(vfs_read(fp, buf, sizeof(buf)), 6);
    fput(fp);

    EXPECT_EQ(vfs_rmdir(NULL, "/vfstest/d"), 0);
    EXPECT_EQ(read_str("/vfstest/abs/f", buf, sizeof(buf)), -ENOENT);
    EXPECT_EQ(vfs_mkdir(NULL, "/vfstest/d", 0755), 0);
}

KERNEL_TEST(vfs_mounts) {
    char         buf[16];
    struct file *fp;

    EXPECT_EQ(vfs_mkdir(NULL, "/vfstest/mnt", 0755), 0);
    EXPECT_EQ(write_str("/vfstest/mnt/under", "under", 0), 0);
    EXPECT_EQ(vfs_mount("ramfs", NULL, "/vfstest/mnt", 0, NULL), 0);

    EXPECT_EQ(read_str("/vfstest/mnt/under", buf, sizeof(buf)), -ENOENT);
    EXPECT_EQ(write_str("/vfstest/mnt/over", "over", 0), 0);
    EXPECT_EQ(read_str("/vfstest/mnt/../mnt/over", buf, sizeof(buf)), 4);
    EXPECT_EQ(read_str("/vfstest/mnt/../a", buf, sizeof(buf)), 2);
    EXPECT_EQ(vfs_rmdir(NULL, "/vfstest/mnt"), -EBUSY);

    EXPECT_EQ(vfs_open(NULL, "/vfstest/mnt/over", O_RDONLY, 0, &fp), 0);
    EXPECT_EQ(vfs_umount("/vfstest/mnt"), -EBUSY);
    fput(fp);

    EXPECT_EQ(vfs_umount("/vfstest/mnt"), 0);
    EXPECT_EQ(read_str("/vfstest/mnt/under", buf, sizeof(buf)), 5);
    EXPECT_EQ(read_str("/vfstest/mnt/over", buf, sizeof(buf)), -ENOENT);
    EXPECT_EQ(vfs_umount("/"), -EBUSY);
}

KERNEL_TEST(vfs_chrdev) {
    char         buf[8];
    struct file *fp;

    memset(buf, 0x55, sizeof(buf));
    EXPECT_EQ(vfs_open(NULL, "/dev/zero", O_RDWR, 0, &fp), 0);
    EXPECT_EQ(vfs_read(fp, buf, sizeof(buf)), (ssize_t) sizeof(buf));
    EXPECT_EQ(buf[0], 0);
    EXPECT_EQ(buf[7], 0);
    fput(fp);

    EXPECT_EQ(vfs_open(NULL, "/dev/null", O_RDWR, 0, &fp), 0);
    EXPECT_EQ(vfs_read(fp, buf, sizeof(buf)), 0);
    EXPECT_EQ(vfs_write(fp, buf, sizeof(buf)), (ssize_t) sizeof(buf));
    fput(fp);

    EXPECT_EQ(vfs_mknod(NULL, "/vfstest/nodev", S_IFCHR | 0600, makedev(200, 0)), 0);
    EXPECT_EQ(vfs_open(NULL, "/vfstest/nodev", O_RDONLY, 0, &fp), -ENXIO);
}
