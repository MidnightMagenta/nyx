#include <nyx/elf.h>
#include <nyx/errno.h>
#include <nyx/exec.h>
#include <nyx/kernel.h>
#include <nyx/string.h>
#include <nyx/types.h>
#include <nyx/vfs.h>
#include <uapi/fcntl.h>

#define hcf()                                                                                                          \
    while (1) { asm volatile("hlt"); }

DEFINE_SUBSYS_LOG(exec_log, "exec", CONFIG_EXEC_LOG_LEVEL);

int do_exec(const char *path, char *const *argv, char *const *envp) {
    struct vnode *vn;
    struct vattr  attr;
    elfhdr        ehdr;
    int           res;

    if ((res = kopen(path, O_RDONLY, 0, &vn))) { return res; }
    if ((res = VOP_GETATTR(vn, &attr))) { goto fail0; }
    if (attr.va_type != VREG || attr.va_size < 0 || ((size_t) attr.va_size < sizeof(Elf64_Ehdr))) {
        res = -ENOEXEC;
        goto fail0;
    }

    if ((res = vn_rdwr(UIO_READ, vn, &ehdr, sizeof(ehdr), 0, NULL))) { goto fail0; }

fail0:
    vrele(vn);
    hcf();
}
