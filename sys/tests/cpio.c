#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/fs/cpio.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/testing.h>
#include <sys/vfs.h>

struct cpio_builder {
    char   buf[4096];
    size_t len;
};

static void cb_pad(struct cpio_builder *cb) {
    while (cb->len & 3) { cb->buf[cb->len++] = '\0'; }
}

static void cb_add(struct cpio_builder *cb,
                   const char          *name,
                   mode_t               mode,
                   const char          *data,
                   size_t               size,
                   unsigned int         rmaj,
                   unsigned int         rmin) {
    size_t namesize = strlen(name) + 1;

    cb->len += sprintf(cb->buf + cb->len,
                       "070701%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx",
                       1ul,
                       (unsigned long) mode,
                       0ul,
                       0ul,
                       1ul,
                       0ul,
                       (unsigned long) size,
                       0ul,
                       0ul,
                       (unsigned long) rmaj,
                       (unsigned long) rmin,
                       (unsigned long) namesize,
                       0ul);
    memcpy(cb->buf + cb->len, name, namesize);
    cb->len += namesize;
    cb_pad(cb);
    if (size) {
        memcpy(cb->buf + cb->len, data, size);
        cb->len += size;
        cb_pad(cb);
    }
}

static void cb_file(struct cpio_builder *cb, const char *name, mode_t perm, const char *data) {
    cb_add(cb, name, S_IFREG | perm, data, strlen(data), 0, 0);
}

static void cb_trailer(struct cpio_builder *cb) {
    cb_add(cb, "TRAILER!!!", 0, NULL, 0, 0, 0);
}

static ssize_t read_rel(const struct path *dir, const char *name, char *buf, size_t len) {
    struct file *fp;
    ssize_t      n;
    int          res;

    if ((res = vfs_open(dir, name, O_RDONLY, 0, &fp))) { return res; }
    n = vfs_read(fp, buf, len - 1);
    fput(fp);

    if (n >= 0) { buf[n] = '\0'; }
    return n;
}

static mode_t mode_of(const struct path *dir, const char *name) {
    struct path p;
    mode_t      mode;

    if (vfs_lookup(dir, name, 0, &p)) { return 0; }
    mode = d_inode(p.p_dent)->i_mode;
    path_put(&p);

    return mode;
}

static void make_dest(struct test_state *state, const char *dir, struct path *out) {
    int res = vfs_mkdir(NULL, dir, 0755);

    if (res && res != -EEXIST) { FAIL("mkdir %s failed: %d", dir, res); }
    ASSERT_OK(vfs_lookup(NULL, dir, LOOKUP_FOLLOW, out));
}

KERNEL_TEST(cpio_unpack_tree) {
    static struct cpio_builder cb;
    struct path                dest, p;
    char                       buf[32];

    ASSERT_NO_FAILURE(make_dest(state, "/cpiotest", &dest));

    cb.len = 0;
    cb_add(&cb, ".", S_IFDIR | 0755, NULL, 0, 0, 0);
    cb_add(&cb, "a", S_IFDIR | 0750, NULL, 0, 0, 0);
    cb_file(&cb, "a/f", 0640, "hello");
    cb_add(&cb, "a/l", S_IFLNK | 0777, "f", 1, 0, 0);
    cb_add(&cb, "./b", S_IFDIR | 0755, NULL, 0, 0, 0);
    cb_file(&cb, "/b/odd", 0600, "xyz");
    cb_file(&cb, "empty", 0644, "");
    cb_add(&cb, "nul", S_IFCHR | 0666, NULL, 0, 1, 3);
    cb_trailer(&cb);
    cb_file(&cb, "after_trailer", 0644, "ignored");

    ASSERT_OK(cpio_unpack(cb.buf, cb.len, &dest));

    EXPECT_EQ(read_rel(&dest, "a/f", buf, sizeof(buf)), 5);
    EXPECT_STREQ(buf, "hello");
    EXPECT_EQ(read_rel(&dest, "a/l", buf, sizeof(buf)), 5);
    EXPECT_EQ(read_rel(&dest, "b/odd", buf, sizeof(buf)), 3);
    EXPECT_STREQ(buf, "xyz");
    EXPECT_EQ(read_rel(&dest, "empty", buf, sizeof(buf)), 0);
    EXPECT_EQ(read_rel(&dest, "after_trailer", buf, sizeof(buf)), -ENOENT);

    EXPECT_EQ(mode_of(&dest, "a"), S_IFDIR | 0750);
    EXPECT_EQ(mode_of(&dest, "a/f"), S_IFREG | 0640);
    EXPECT_TRUE(S_ISLNK(mode_of(&dest, "a/l")));
    EXPECT_TRUE(S_ISCHR(mode_of(&dest, "nul")));

    if (vfs_lookup(&dest, "nul", 0, &p) == 0) {
        EXPECT_EQ(d_inode(p.p_dent)->i_rdev, makedev(1, 3));
        path_put(&p);
    } else {
        ADD_FAILURE("device node was not created");
    }

    cb.len = 0;
    cb_add(&cb, "a", S_IFDIR | 0750, NULL, 0, 0, 0);
    cb_file(&cb, "a/f", 0640, "bye");
    cb_trailer(&cb);
    EXPECT_OK(cpio_unpack(cb.buf, cb.len, &dest));
    EXPECT_EQ(read_rel(&dest, "a/f", buf, sizeof(buf)), 3);
    EXPECT_STREQ(buf, "bye");

    path_put(&dest);
}

KERNEL_TEST(cpio_unpack_malformed) {
    static struct cpio_builder cb;
    struct path                dest;

    ASSERT_NO_FAILURE(make_dest(state, "/cpiotest/bad", &dest));

    EXPECT_EQ(cpio_unpack(cb.buf, 0, &dest), -EINVAL);

    /* no trailer */
    cb.len = 0;
    cb_file(&cb, "x", 0644, "data");
    EXPECT_EQ(cpio_unpack(cb.buf, cb.len, &dest), -EINVAL);

    /* bad magic */
    cb.len = 0;
    cb_trailer(&cb);
    cb.buf[5] = '9';
    EXPECT_EQ(cpio_unpack(cb.buf, cb.len, &dest), -EINVAL);

    /* non-hex header field */
    cb.len = 0;
    cb_trailer(&cb);
    cb.buf[6 + 8 * 2] = 'g';
    EXPECT_EQ(cpio_unpack(cb.buf, cb.len, &dest), -EINVAL);

    /* header cut short */
    cb.len = 0;
    cb_trailer(&cb);
    EXPECT_EQ(cpio_unpack(cb.buf, 100, &dest), -EINVAL);

    /* file data runs past the end of the archive */
    cb.len = 0;
    cb_file(&cb, "y", 0644, "0123456789abcdef");
    EXPECT_EQ(cpio_unpack(cb.buf, cb.len - 8, &dest), -EINVAL);

    /* name not NUL terminated (namesize claims one byte less than written) */
    cb.len = 0;
    cb_trailer(&cb);
    memcpy(cb.buf + 6 + 8 * 11, "0000000a", 8);
    EXPECT_EQ(cpio_unpack(cb.buf, cb.len, &dest), -EINVAL);

    path_put(&dest);
}
