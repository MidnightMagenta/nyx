#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/testing.h>
#include <sys/vfs.h>

static void fake_proc_init(struct test_state *state, struct process *p) {
    memset(p, 0, sizeof(*p));
    ASSERT_OK(fdinit(p));
    ASSERT_NOT_NULL(p->p_fd);
}

static struct file *open_null(struct test_state *state) {
    struct file *fp = NULL;

    EXPECT_OK(vfs_open(NULL, "/dev/null", O_RDWR, 0, &fp));
    return fp;
}

static int file_refs(struct file *fp) {
    return refcount_get(&fp->f_refs);
}

KERNEL_TEST(filedesc_install_get_close) {
    struct process p;
    struct file   *fp, *got;
    int            fd = -1;

    ASSERT_NO_FAILURE(fake_proc_init(state, &p));
    fp = open_null(state);
    ASSERT_NOT_NULL(fp);
    EXPECT_EQ(file_refs(fp), 1);

    ASSERT_OK(fd_install(&p, fp, 0, &fd));
    EXPECT_EQ(fd, 0);
    EXPECT_EQ(file_refs(fp), 1);

    got = fget(&p, fd);
    EXPECT_EQ(got, fp);
    EXPECT_EQ(file_refs(fp), 2);
    if (got) { fput(got); }

    EXPECT_NULL(fget(&p, -1));
    EXPECT_NULL(fget(&p, 1));
    EXPECT_NULL(fget(&p, 100000));

    fhold(fp);
    EXPECT_OK(fd_close(&p, fd));
    EXPECT_EQ(file_refs(fp), 1);
    EXPECT_NULL(fget(&p, fd));
    EXPECT_EQ(fd_close(&p, fd), -EBADF);
    EXPECT_EQ(fd_close(&p, -1), -EBADF);
    EXPECT_EQ(kern_close(&p, 12345), -EBADF);
    fput(fp);

    fdfree(&p);
    EXPECT_NULL(p.p_fd);
}

KERNEL_TEST(filedesc_lowest_free) {
    struct process p;
    struct file   *fp;
    int            fds[3], fd;

    ASSERT_NO_FAILURE(fake_proc_init(state, &p));
    fp = open_null(state);
    ASSERT_NOT_NULL(fp);

    for (int i = 0; i < 3; i++) {
        fhold(fp);
        EXPECT_OK(fd_install(&p, fp, 0, &fds[i]));
        EXPECT_EQ(fds[i], i);
    }

    EXPECT_OK(fd_close(&p, 1));
    fhold(fp);
    EXPECT_OK(fd_install(&p, fp, 0, &fd));
    EXPECT_EQ(fd, 1);

    fhold(fp);
    EXPECT_OK(fd_install(&p, fp, 0, &fd));
    EXPECT_EQ(fd, 3);

    EXPECT_EQ(file_refs(fp), 5);
    fdfree(&p);
    EXPECT_EQ(file_refs(fp), 1);
    fput(fp);
}

KERNEL_TEST(filedesc_grow) {
    struct process p;
    struct file   *fp;
    int            fd, initial;

    ASSERT_NO_FAILURE(fake_proc_init(state, &p));
    initial = p.p_fd->fd_nfiles;
    fp      = open_null(state);
    ASSERT_NOT_NULL(fp);

    for (int i = 0; i < 3 * initial; i++) {
        fhold(fp);
        if (fd_install(&p, fp, 0, &fd)) {
            fput(fp);
            ADD_FAILURE("fd_install %d failed", i);
            break;
        }
        if (fd != i) {
            ADD_FAILURE("install %d got fd %d", i, fd);
            break;
        }
    }

    EXPECT_GE(p.p_fd->fd_nfiles, 3 * initial);
    EXPECT_LE(p.p_fd->fd_nfiles, OPEN_MAX);

    for (int i = 0; i < 3 * initial; i += initial - 1) {
        struct file *got = fget(&p, i);
        EXPECT_EQ(got, fp);
        if (got) { fput(got); }
    }

    EXPECT_EQ(file_refs(fp), 3 * initial + 1);
    fdfree(&p);
    EXPECT_EQ(file_refs(fp), 1);
    fput(fp);
}

KERNEL_TEST(filedesc_dup) {
    struct process p;
    struct file   *fp;
    int            fd, newfd = -1;

    ASSERT_NO_FAILURE(fake_proc_init(state, &p));
    fp = open_null(state);
    ASSERT_NOT_NULL(fp);
    ASSERT_OK(fd_install(&p, fp, FD_CLOEXEC, &fd));

    EXPECT_OK(fd_dup(&p, fd, &newfd));
    EXPECT_EQ(newfd, 1);
    EXPECT_EQ(file_refs(fp), 2);
    EXPECT_EQ(p.p_fd->fd_files[fd].fe_flags, FD_CLOEXEC);
    EXPECT_EQ(p.p_fd->fd_files[newfd].fe_flags, 0);

    EXPECT_OK(kern_dup(&p, newfd, &newfd));
    EXPECT_EQ(newfd, 2);
    EXPECT_EQ(file_refs(fp), 3);

    EXPECT_EQ(fd_dup(&p, 7, &newfd), -EBADF);
    EXPECT_EQ(fd_dup(&p, -1, &newfd), -EBADF);
    EXPECT_EQ(file_refs(fp), 3);

    EXPECT_OK(fd_close(&p, 0));
    EXPECT_EQ(file_refs(fp), 2);

    fdfree(&p);
}

KERNEL_TEST(filedesc_copy) {
    struct process   parent, child;
    struct file     *a, *b;
    struct filedesc *copy;
    int              fa, fb;

    ASSERT_NO_FAILURE(fake_proc_init(state, &parent));
    a = open_null(state);
    b = open_null(state);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    ASSERT_OK(fd_install(&parent, a, FD_CLOEXEC, &fa));
    ASSERT_OK(fd_install(&parent, b, FD_CLOFORK, &fb));

    copy = fdcopy(parent.p_fd);
    ASSERT_NOT_NULL(copy);
    memset(&child, 0, sizeof(child));
    child.p_fd = copy;

    EXPECT_EQ(copy->fd_nfiles, parent.p_fd->fd_nfiles);
    EXPECT_EQ(refcount_get(&copy->fd_refs), 1);

    EXPECT_EQ(copy->fd_files[fa].fe_file, a);
    EXPECT_EQ(copy->fd_files[fa].fe_flags, FD_CLOEXEC);
    EXPECT_NULL(copy->fd_files[fb].fe_file);
    EXPECT_EQ(file_refs(a), 2);
    EXPECT_EQ(file_refs(b), 1);

    EXPECT_OK(fd_close(&child, fa));
    EXPECT_EQ(file_refs(a), 1);
    EXPECT_EQ(parent.p_fd->fd_files[fa].fe_file, a);

    fdfree(&child);
    fdfree(&parent);
}

KERNEL_TEST(filedesc_root_cwd) {
    struct process p;
    struct path    dev, got;
    int            refs;

    ASSERT_NO_FAILURE(fake_proc_init(state, &p));
    ASSERT_OK(vfs_lookup(NULL, "/dev", LOOKUP_FOLLOW, &dev));
    refs = refcount_get(&dev.p_dent->d_refs);

    fd_get_cwd(p.p_fd, &got);
    EXPECT_NULL(got.p_mnt);
    EXPECT_NULL(got.p_dent);

    fd_set_cwd(p.p_fd, &dev);
    EXPECT_EQ(refcount_get(&dev.p_dent->d_refs), refs + 1);

    fd_get_cwd(p.p_fd, &got);
    EXPECT_EQ(got.p_dent, dev.p_dent);
    EXPECT_EQ(got.p_mnt, dev.p_mnt);
    EXPECT_EQ(refcount_get(&dev.p_dent->d_refs), refs + 2);
    path_put(&got);

    fd_set_root(p.p_fd, &dev);
    fd_get_root(p.p_fd, &got);
    EXPECT_EQ(got.p_dent, dev.p_dent);
    path_put(&got);
    EXPECT_EQ(refcount_get(&dev.p_dent->d_refs), refs + 2);

    struct process child;
    memset(&child, 0, sizeof(child));
    child.p_fd = fdcopy(p.p_fd);
    ASSERT_NOT_NULL(child.p_fd);
    EXPECT_EQ(refcount_get(&dev.p_dent->d_refs), refs + 4);
    fdfree(&child);

    fdfree(&p);
    EXPECT_EQ(refcount_get(&dev.p_dent->d_refs), refs);
    path_put(&dev);
}
