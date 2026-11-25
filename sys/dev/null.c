#include <sys/initcall.h>
#include <sys/posix_types.h>
#include <sys/stat.h>
#include <sys/stddef.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#define NULL_RDEV makedev(1, 3)

static ssize_t null_read(struct file *fp, char *buf, size_t cnt, off_t *pos) {
    (void) fp;
    (void) buf;
    (void) cnt;
    (void) pos;
    return 0;
}

static ssize_t null_write(struct file *fp, const char *buf, size_t cnt, off_t *pos) {
    (void) fp;
    (void) buf;
    (void) pos;
    return (ssize_t) cnt;
}

static const struct fileops null_fops = {
        .fop_read  = null_read,
        .fop_write = null_write,
};

DEFINE_INITCALL(init_nulldev) {
    BUG_ON(chrdev_register(NULL_RDEV, "null", &null_fops));
    BUG_ON(vfs_mknod(NULL, "/dev/null", S_IFCHR | 0666, NULL_RDEV));
}
