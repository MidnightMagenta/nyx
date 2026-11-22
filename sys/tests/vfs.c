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

    ASSERT_EQ(vfs_lookup(NULL, "/sbin/init", LOOKUP_FOLLOW, &p), 0);
    ASSERT_TRUE(S_ISREG(d_inode(p.p_dent)->i_mode));
    ASSERT_TRUE(d_inode(p.p_dent)->i_size > 0);
    path_put(&p);
}

KERNEL_TEST(vfs_file_read_write) {
    char         buf[32];
    struct file *fp;

    ASSERT_EQ(vfs_mkdir(NULL, "/vfstest", 0755), 0);
    ASSERT_EQ(vfs_mkdir(NULL, "/vfstest", 0755), -EEXIST);
    ASSERT_EQ(write_str("/vfstest/a", "hello", O_EXCL), 0);
    ASSERT_EQ(write_str("/vfstest/a", "x", O_EXCL), -EEXIST);
    ASSERT_EQ(read_str("/vfstest/a", buf, sizeof(buf)), 5);
    ASSERT_EQ(strcmp(buf, "hello"), 0);

    ASSERT_EQ(write_str("/vfstest/a", " world", O_APPEND), 0);
    ASSERT_EQ(read_str("/vfstest/./../vfstest//a", buf, sizeof(buf)), 11);
    ASSERT_EQ(strcmp(buf, "hello world"), 0);

    ASSERT_EQ(vfs_open(NULL, "/vfstest/a", O_RDWR, 0, &fp), 0);
    ASSERT_EQ(vfs_llseek(fp, 6, SEEK_SET), 6);
    ASSERT_EQ(vfs_read(fp, buf, 5), 5);
    ASSERT_EQ(memcmp(buf, "world", 5), 0);
    ASSERT_EQ(vfs_llseek(fp, -5, SEEK_END), 6);
    ASSERT_EQ(vfs_pwrite(fp, "W", 1, 6), 1);
    ASSERT_EQ(vfs_pread(fp, buf, 5, 6), 5);
    ASSERT_EQ(memcmp(buf, "World", 5), 0);
    fput(fp);

    ASSERT_EQ(write_str("/vfstest/a", "hi", O_TRUNC), 0);
    ASSERT_EQ(read_str("/vfstest/a", buf, sizeof(buf)), 2);
    ASSERT_EQ(vfs_open(NULL, "/vfstest/a/", O_RDONLY, 0, &fp), -ENOTDIR);
    ASSERT_EQ(vfs_open(NULL, "/vfstest", O_WRONLY, 0, &fp), -EISDIR);
    ASSERT_EQ(vfs_open(NULL, "/vfstest/missing", O_RDONLY, 0, &fp), -ENOENT);
}

KERNEL_TEST(vfs_large_file) {
    static char  pattern[3 * 4096 + 123];
    static char  back[sizeof(pattern)];
    struct file *fp;

    for (size_t i = 0; i < sizeof(pattern); i++) { pattern[i] = (char) (i * 7); }

    ASSERT_EQ(vfs_open(NULL, "/vfstest/big", O_RDWR | O_CREAT, 0644, &fp), 0);
    ASSERT_EQ(vfs_pwrite(fp, pattern, sizeof(pattern), 0), (ssize_t) sizeof(pattern));
    ASSERT_EQ(vfs_pread(fp, back, sizeof(back), 0), (ssize_t) sizeof(back));
    ASSERT_EQ(memcmp(pattern, back, sizeof(pattern)), 0);

    ASSERT_EQ(vfs_truncate(f_inode(fp), 5000), 0);
    ASSERT_EQ(vfs_truncate(f_inode(fp), 9000), 0);
    ASSERT_EQ(vfs_pread(fp, back, sizeof(back), 0), 9000);
    ASSERT_EQ(memcmp(pattern, back, 5000), 0);
    ASSERT_EQ(back[5000], 0);
    ASSERT_EQ(back[8999], 0);
    fput(fp);
}

KERNEL_TEST(vfs_symlinks) {
    char        buf[32];
    struct path p;

    ASSERT_EQ(vfs_mkdir(NULL, "/vfstest/d", 0755), 0);
    ASSERT_EQ(write_str("/vfstest/d/f", "linked", 0), 0);
    ASSERT_EQ(vfs_symlink(NULL, "/vfstest/rel", "d/f"), 0);
    ASSERT_EQ(vfs_symlink(NULL, "/vfstest/abs", "/vfstest/d"), 0);
    ASSERT_EQ(vfs_symlink(NULL, "/vfstest/loop", "/vfstest/loop"), 0);

    ASSERT_EQ(read_str("/vfstest/rel", buf, sizeof(buf)), 6);
    ASSERT_EQ(strcmp(buf, "linked"), 0);
    ASSERT_EQ(read_str("/vfstest/abs/f", buf, sizeof(buf)), 6);
    ASSERT_EQ(read_str("/vfstest/abs/../a", buf, sizeof(buf)), 2);
    ASSERT_EQ(read_str("/vfstest/loop", buf, sizeof(buf)), -ELOOP);

    ASSERT_EQ(vfs_lookup(NULL, "/vfstest/rel", 0, &p), 0);
    ASSERT_TRUE(S_ISLNK(d_inode(p.p_dent)->i_mode));
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

    ASSERT_EQ(vfs_open(NULL, "/vfstest/d", O_RDONLY | O_DIRECTORY, 0, &fp), 0);
    ASSERT_EQ(vfs_iterate(fp, &l.ctx), 0);
    ASSERT_EQ(l.count, 3);
    ASSERT_TRUE(l.saw_f);

    l.count = 0;
    ASSERT_EQ(vfs_iterate(fp, &l.ctx), 0);
    ASSERT_EQ(l.count, 0);
    fput(fp);
}

KERNEL_TEST(vfs_remove) {
    char         buf[16];
    struct file *fp;

    ASSERT_EQ(vfs_rmdir(NULL, "/vfstest/d"), -ENOTEMPTY);
    ASSERT_EQ(vfs_unlink(NULL, "/vfstest/d"), -EISDIR);
    ASSERT_EQ(vfs_rmdir(NULL, "/vfstest/d/f"), -ENOTDIR);

    ASSERT_EQ(vfs_open(NULL, "/vfstest/d/f", O_RDONLY, 0, &fp), 0);
    ASSERT_EQ(vfs_unlink(NULL, "/vfstest/d/f"), 0);
    ASSERT_EQ(vfs_open(NULL, "/vfstest/d/f", O_RDONLY, 0, &fp), -ENOENT);
    ASSERT_EQ(vfs_read(fp, buf, sizeof(buf)), 6);
    fput(fp);

    ASSERT_EQ(vfs_rmdir(NULL, "/vfstest/d"), 0);
    ASSERT_EQ(read_str("/vfstest/abs/f", buf, sizeof(buf)), -ENOENT);
    ASSERT_EQ(vfs_mkdir(NULL, "/vfstest/d", 0755), 0);
}

KERNEL_TEST(vfs_mounts) {
    char         buf[16];
    struct file *fp;

    ASSERT_EQ(vfs_mkdir(NULL, "/vfstest/mnt", 0755), 0);
    ASSERT_EQ(write_str("/vfstest/mnt/under", "under", 0), 0);
    ASSERT_EQ(vfs_mount("ramfs", NULL, "/vfstest/mnt", 0, NULL), 0);

    ASSERT_EQ(read_str("/vfstest/mnt/under", buf, sizeof(buf)), -ENOENT);
    ASSERT_EQ(write_str("/vfstest/mnt/over", "over", 0), 0);
    ASSERT_EQ(read_str("/vfstest/mnt/../mnt/over", buf, sizeof(buf)), 4);
    ASSERT_EQ(read_str("/vfstest/mnt/../a", buf, sizeof(buf)), 2);
    ASSERT_EQ(vfs_rmdir(NULL, "/vfstest/mnt"), -EBUSY);

    ASSERT_EQ(vfs_open(NULL, "/vfstest/mnt/over", O_RDONLY, 0, &fp), 0);
    ASSERT_EQ(vfs_umount("/vfstest/mnt"), -EBUSY);
    fput(fp);

    ASSERT_EQ(vfs_umount("/vfstest/mnt"), 0);
    ASSERT_EQ(read_str("/vfstest/mnt/under", buf, sizeof(buf)), 5);
    ASSERT_EQ(read_str("/vfstest/mnt/over", buf, sizeof(buf)), -ENOENT);
    ASSERT_EQ(vfs_umount("/"), -EBUSY);
}

KERNEL_TEST(vfs_chrdev) {
    char         buf[8];
    struct file *fp;

    memset(buf, 0x55, sizeof(buf));
    ASSERT_EQ(vfs_open(NULL, "/dev/zero", O_RDWR, 0, &fp), 0);
    ASSERT_EQ(vfs_read(fp, buf, sizeof(buf)), (ssize_t) sizeof(buf));
    ASSERT_EQ(buf[0], 0);
    ASSERT_EQ(buf[7], 0);
    fput(fp);

    ASSERT_EQ(vfs_open(NULL, "/dev/null", O_RDWR, 0, &fp), 0);
    ASSERT_EQ(vfs_read(fp, buf, sizeof(buf)), 0);
    ASSERT_EQ(vfs_write(fp, buf, sizeof(buf)), (ssize_t) sizeof(buf));
    fput(fp);

    ASSERT_EQ(vfs_mknod(NULL, "/vfstest/nodev", S_IFCHR | 0600, makedev(200, 0)), 0);
    ASSERT_EQ(vfs_open(NULL, "/vfstest/nodev", O_RDONLY, 0, &fp), -ENXIO);
}

KERNEL_TEST(vfs_access_modes) {
    char         buf[8];
    struct file *fp;

    ASSERT_OK(write_str("/vfstest/modes", "abc", O_TRUNC));

    ASSERT_OK(vfs_open(NULL, "/vfstest/modes", O_RDONLY, 0, &fp));
    EXPECT_EQ(vfs_write(fp, "x", 1), -EBADF);
    EXPECT_EQ(vfs_pwrite(fp, "x", 1, 0), -EBADF);
    EXPECT_EQ(vfs_read(fp, buf, sizeof(buf)), 3);
    fput(fp);

    ASSERT_OK(vfs_open(NULL, "/vfstest/modes", O_WRONLY, 0, &fp));
    EXPECT_EQ(vfs_read(fp, buf, sizeof(buf)), -EBADF);
    EXPECT_EQ(vfs_pread(fp, buf, 1, 0), -EBADF);
    EXPECT_EQ(vfs_pwrite(fp, "x", 1, -1), -EINVAL);
    fput(fp);

    EXPECT_EQ(vfs_open(NULL, "/vfstest/modes", O_ACCMODE, 0, &fp), -EINVAL);
    EXPECT_EQ(vfs_open(NULL, "/vfstest/modes", O_RDONLY | O_DIRECTORY, 0, &fp), -ENOTDIR);
    EXPECT_EQ(vfs_open(NULL, "/vfstest/missing/x", O_RDONLY | O_CREAT, 0644, &fp), -ENOENT);

    ASSERT_OK(vfs_open(NULL, "/vfstest", O_RDONLY, 0, &fp));
    EXPECT_EQ(vfs_read(fp, buf, sizeof(buf)), -EISDIR);
    fput(fp);

    ASSERT_OK(vfs_open(NULL, "/vfstest/modes", O_RDONLY, 0, &fp));
    struct dir_ctx ctx = {.dc_callback = NULL};
    EXPECT_EQ(vfs_iterate(fp, &ctx), -ENOTDIR);
    fput(fp);

    EXPECT_OK(vfs_unlink(NULL, "/vfstest/modes"));
}

KERNEL_TEST(vfs_llseek) {
    char         buf[8];
    struct file *fp;

    ASSERT_OK(write_str("/vfstest/seek", "0123456789", O_TRUNC));
    ASSERT_OK(vfs_open(NULL, "/vfstest/seek", O_RDWR, 0, &fp));

    EXPECT_EQ(vfs_llseek(fp, 3, SEEK_SET), 3);
    EXPECT_EQ(vfs_llseek(fp, 2, SEEK_CUR), 5);
    EXPECT_EQ(vfs_llseek(fp, -1, SEEK_CUR), 4);
    EXPECT_EQ(vfs_read(fp, buf, 2), 2);
    EXPECT_MEMEQ(buf, "45", 2);
    EXPECT_EQ(vfs_llseek(fp, 0, SEEK_CUR), 6);
    EXPECT_EQ(vfs_llseek(fp, 0, SEEK_END), 10);

    EXPECT_EQ(vfs_llseek(fp, -11, SEEK_END), -EINVAL);
    EXPECT_EQ(vfs_llseek(fp, -1, SEEK_SET), -EINVAL);
    EXPECT_EQ(vfs_llseek(fp, 0, 42), -EINVAL);
    EXPECT_EQ(vfs_llseek(fp, OFF_MAX, SEEK_CUR), -EINVAL);
    EXPECT_EQ(vfs_llseek(fp, 0, SEEK_CUR), 10);

    EXPECT_EQ(vfs_llseek(fp, 20, SEEK_SET), 20);
    EXPECT_EQ(vfs_read(fp, buf, sizeof(buf)), 0);
    EXPECT_EQ(vfs_write(fp, "X", 1), 1);
    EXPECT_EQ(f_inode(fp)->i_size, 21);
    EXPECT_EQ(vfs_pread(fp, buf, 3, 9), 3);
    EXPECT_MEMEQ(buf, "9\0\0", 3);
    fput(fp);

    EXPECT_OK(vfs_unlink(NULL, "/vfstest/seek"));
}

KERNEL_TEST(vfs_relative_paths) {
    char         buf[16];
    struct path  dir;
    struct file *fp;

    ASSERT_OK(vfs_mkdir(NULL, "/vfstest/rel_dir", 0755));
    ASSERT_OK(vfs_lookup(NULL, "/vfstest/rel_dir", LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &dir));

    EXPECT_OK(vfs_mkdir(&dir, "sub", 0755));
    EXPECT_OK(vfs_open(&dir, "sub/file", O_WRONLY | O_CREAT, 0644, &fp));
    if (!TEST_HAS_FAILED()) {
        EXPECT_EQ(vfs_write(fp, "relative", 8), 8);
        fput(fp);
    }

    EXPECT_EQ(read_str("/vfstest/rel_dir/sub/file", buf, sizeof(buf)), 8);
    EXPECT_STREQ(buf, "relative");

    EXPECT_OK(vfs_open(&dir, "/vfstest/rel_dir/sub/file", O_RDONLY, 0, &fp));
    if (!TEST_HAS_FAILED()) { fput(fp); }

    EXPECT_EQ(read_str("/../../vfstest/rel_dir/sub/file", buf, sizeof(buf)), 8);

    EXPECT_EQ(vfs_lookup(NULL, "/vfstest/a", LOOKUP_DIRECTORY, &(struct path){0}), -ENOTDIR);

    EXPECT_OK(vfs_unlink(&dir, "sub/file"));
    EXPECT_OK(vfs_rmdir(&dir, "sub"));
    path_put(&dir);
    EXPECT_OK(vfs_rmdir(NULL, "/vfstest/rel_dir"));
}

KERNEL_TEST(vfs_nofollow) {
    struct file *fp;

    ASSERT_OK(write_str("/vfstest/nf_target", "t", O_TRUNC));
    ASSERT_OK(vfs_symlink(NULL, "/vfstest/nf_link", "nf_target"));

    EXPECT_EQ(vfs_open(NULL, "/vfstest/nf_link", O_RDONLY | O_NOFOLLOW, 0, &fp), -ELOOP);
    EXPECT_OK(vfs_open(NULL, "/vfstest/nf_link", O_RDONLY, 0, &fp));
    if (!TEST_HAS_FAILED()) { fput(fp); }

    EXPECT_EQ(vfs_symlink(NULL, "/vfstest/nf_link", "other"), -EEXIST);
    EXPECT_OK(vfs_unlink(NULL, "/vfstest/nf_link"));
    EXPECT_OK(vfs_unlink(NULL, "/vfstest/nf_target"));
}

#define TEST_CHRDEV makedev(201, 7)

static int test_chrdev_opens;

static int test_chrdev_open(struct inode *in, struct file *fp) {
    (void) in;
    (void) fp;
    test_chrdev_opens++;
    return 0;
}

static ssize_t test_chrdev_read(struct file *fp, char *buf, size_t cnt, off_t *pos) {
    (void) fp;
    for (size_t i = 0; i < cnt; i++) { buf[i] = (char) ('a' + (*pos + i) % 26); }
    *pos += cnt;
    return cnt;
}

static const struct fileops test_chrdev_fops = {
        .fop_open = test_chrdev_open,
        .fop_read = test_chrdev_read,
};

KERNEL_TEST(vfs_chrdev_register) {
    char         buf[4];
    struct file *fp;

    test_chrdev_opens = 0;

    ASSERT_OK(chrdev_register(TEST_CHRDEV, "test", &test_chrdev_fops));
    EXPECT_EQ(chrdev_register(TEST_CHRDEV, "test", &test_chrdev_fops), -EEXIST);
    ASSERT_OK(vfs_mknod(NULL, "/vfstest/testdev", S_IFCHR | 0600, TEST_CHRDEV));

    ASSERT_OK(vfs_open(NULL, "/vfstest/testdev", O_RDONLY, 0, &fp));
    EXPECT_EQ(test_chrdev_opens, 1);
    EXPECT_EQ(fp->f_op, &test_chrdev_fops);
    EXPECT_EQ(vfs_read(fp, buf, 3), 3);
    EXPECT_MEMEQ(buf, "abc", 3);
    EXPECT_EQ(vfs_read(fp, buf, 2), 2);
    EXPECT_MEMEQ(buf, "de", 2);
    EXPECT_EQ(vfs_write(fp, buf, 1), -EBADF);
    fput(fp);

    ASSERT_OK(vfs_open(NULL, "/vfstest/testdev", O_WRONLY, 0, &fp));
    EXPECT_EQ(vfs_write(fp, buf, 1), -EINVAL);
    fput(fp);

    EXPECT_OK(chrdev_unregister(TEST_CHRDEV));
    EXPECT_EQ(chrdev_unregister(TEST_CHRDEV), -ENOENT);
    EXPECT_EQ(vfs_open(NULL, "/vfstest/testdev", O_RDONLY, 0, &fp), -ENXIO);
    EXPECT_OK(vfs_unlink(NULL, "/vfstest/testdev"));
}
