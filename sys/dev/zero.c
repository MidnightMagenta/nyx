#include <sys/initcall.h>
#include <sys/posix_types.h>
#include <sys/stat.h>
#include <sys/stddef.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#define ZERO_RDEV makedev(1, 5)

static ssize_t zero_read(struct file *fp, char *buf, size_t cnt, off_t *pos) {
    (void) fp;
    (void) pos;

    memset(buf, 0, cnt);
    return (ssize_t) cnt;
}

static ssize_t zero_write(struct file *fp, const char *buf, size_t cnt, off_t *pos) {
    (void) fp;
    (void) buf;
    (void) pos;
    return (ssize_t) cnt;
}

static const struct fileops zero_fops = {
        .fop_read  = zero_read,
        .fop_write = zero_write,
};

DEFINE_INITCALL(init_zerodev) {
    BUG_ON(chrdev_register(ZERO_RDEV, "zero", &zero_fops));
    BUG_ON(vfs_mknod(NULL, "/dev/zero", S_IFCHR | 0666, ZERO_RDEV));
}
