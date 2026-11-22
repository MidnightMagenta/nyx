#include <sys/early_serial.h>
#include <sys/initcall.h>
#include <sys/posix_types.h>
#include <sys/stat.h>
#include <sys/stddef.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#define CONSOLE_RDEV makedev(5, 1)

static ssize_t console_read(struct file *fp, char *buf, size_t cnt, off_t *pos) {
    (void) fp;
    (void) buf;
    (void) cnt;
    (void) pos;
    return 0;
}

static ssize_t console_write(struct file *fp, const char *buf, size_t cnt, off_t *pos) {
    (void) fp;
    (void) pos;

    for (size_t i = 0; i < cnt; i++) { early_serial_putc(buf[i]); }

    return (ssize_t) cnt;
}

static const struct fileops console_fops = {
        .fop_read  = console_read,
        .fop_write = console_write,
};

DEFINE_INITCALL(init_consoledev) {
    BUG_ON(chrdev_register(CONSOLE_RDEV, "console", &console_fops));
    BUG_ON(vfs_mknod(NULL, "/dev/console", S_IFCHR | 0666, CONSOLE_RDEV));
}
