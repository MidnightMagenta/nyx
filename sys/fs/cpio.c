#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/fs/cpio.h>
#include <sys/kernel.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/mm_types.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/vfs.h>

#define CPIO_HDR_LEN   110
#define CPIO_MAGIC_LEN 6
#define CPIO_TRAILER   "TRAILER!!!"

enum {
    CPIO_INO,
    CPIO_MODE,
    CPIO_UID,
    CPIO_GID,
    CPIO_NLINK,
    CPIO_MTIME,
    CPIO_FILESIZE,
    CPIO_DEVMAJOR,
    CPIO_DEVMINOR,
    CPIO_RDEVMAJOR,
    CPIO_RDEVMINOR,
    CPIO_NAMESIZE,
    CPIO_CHECK,
    CPIO_NFIELDS,
};

static inline size_t align4(size_t x) {
    return (x + 3) & ~(size_t) 3;
}

static int parse_hex8(const char *p, u32 *out) {
    u32 v = 0;

    for (int i = 0; i < 8; i++) {
        char c = p[i];

        v <<= 4;
        if (c >= '0' && c <= '9') {
            v |= (u32) (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            v |= (u32) (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            v |= (u32) (c - 'A' + 10);
        } else {
            return -EINVAL;
        }
    }

    *out = v;
    return 0;
}

static int write_file(const struct path *dest, const char *name, mode_t mode, const char *data, size_t size) {
    struct file *fp;
    size_t       done = 0;
    int          res;

    if ((res = vfs_open(dest, name, O_WRONLY | O_CREAT | O_TRUNC, mode, &fp))) { return res; }

    while (done < size) {
        ssize_t n = vfs_write(fp, data + done, size - done);

        if (n <= 0) {
            res = n ? (int) n : -EIO;
            break;
        }
        done += (size_t) n;
    }

    fput(fp);
    return res;
}

static int make_symlink(const struct path *dest, const char *name, const char *data, size_t size) {
    char *target;
    int   res;

    if (size == 0 || size >= PATH_MAX) { return -EINVAL; }

    target = kmalloc(size + 1, M_SLEEPOK);
    if (!target) { return -ENOMEM; }

    memcpy(target, data, size);
    target[size] = '\0';

    res = vfs_symlink(dest, name, target);
    kfree(target);

    return res;
}

static int unpack_entry(const struct path *dest, const char *name, const u32 *f, const char *data) {
    mode_t mode = (mode_t) f[CPIO_MODE];
    mode_t perm = mode & ~S_IFMT;
    int    res;

    switch (mode & S_IFMT) {
        case S_IFDIR:
            res = vfs_mkdir(dest, name, perm);
            return res == -EEXIST ? 0 : res;
        case S_IFREG:
            return write_file(dest, name, perm, data, f[CPIO_FILESIZE]);
        case S_IFLNK:
            return make_symlink(dest, name, data, f[CPIO_FILESIZE]);
        case S_IFCHR:
        case S_IFBLK:
        case S_IFIFO:
        case S_IFSOCK:
            return vfs_mknod(dest, name, mode, makedev(f[CPIO_RDEVMAJOR], f[CPIO_RDEVMINOR]));
        default:
            return -EINVAL;
    }
}

int cpio_unpack(const void *base, size_t len, const struct path *dest) {
    const char *archive = base;
    size_t      off     = 0;

    while (off + CPIO_HDR_LEN <= len) {
        const char *hdr = archive + off;
        const char *name, *data;
        u32         f[CPIO_NFIELDS];
        size_t      namesize, filesize;
        int         res;

        if (memcmp(hdr, "070701", CPIO_MAGIC_LEN) && memcmp(hdr, "070702", CPIO_MAGIC_LEN)) { return -EINVAL; }

        for (int i = 0; i < CPIO_NFIELDS; i++) {
            if (parse_hex8(hdr + CPIO_MAGIC_LEN + i * 8, &f[i])) { return -EINVAL; }
        }

        namesize = f[CPIO_NAMESIZE];
        filesize = f[CPIO_FILESIZE];
        if (namesize == 0 || namesize > len - off - CPIO_HDR_LEN) { return -EINVAL; }

        name = hdr + CPIO_HDR_LEN;
        if (name[namesize - 1] != '\0') { return -EINVAL; }

        off = align4(off + CPIO_HDR_LEN + namesize);
        if (off > len || filesize > len - off) { return -EINVAL; }

        data = archive + off;
        off  = align4(off + filesize);

        if (!strcmp(name, CPIO_TRAILER)) { return 0; }

        while (name[0] == '.' && name[1] == '/') { name += 2; }
        while (name[0] == '/') { name++; }
        if (name[0] == '\0' || !strcmp(name, ".")) { continue; }

        if ((res = unpack_entry(dest, name, f, data))) {
            pr_warn(log_generic, "cpio: failed to unpack '%s': %d\n", name, res);
        }
    }

    return -EINVAL;
}
