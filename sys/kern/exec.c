#include <sys/elf.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vfs.h>

#include <asi/system.h>

DEFINE_SUBSYS_LOG(exec_log, "exec", CONFIG_EXEC_LOG_LEVEL);

int do_exec(const char *path, char *const *argv, char *const *envp) {
    struct file  *fp;
    struct inode *inode;
    elfhdr        ehdr;
    ssize_t       n;
    int           res;

    if ((res = vfs_open(NULL, path, O_RDONLY, 0, &fp))) { return res; }

    inode = f_inode(fp);
    if (!S_ISREG(inode->i_mode) || inode->i_size < 0 || ((size_t) inode->i_size < sizeof(Elf64_Ehdr))) {
        res = -ENOEXEC;
        goto fail0;
    }

    n = vfs_pread(fp, &ehdr, sizeof(ehdr), 0);
    if (n != sizeof(ehdr)) {
        res = n < 0 ? (int) n : -ENOEXEC;
        goto fail0;
    }

fail0:
    fput(fp);
    hcf();
}
