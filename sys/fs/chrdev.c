#include <sys/errno.h>
#include <sys/list.h>
#include <sys/mm/kmalloc.h>
#include <sys/mm/mm_types.h>
#include <sys/spinlock.h>
#include <sys/vfs.h>

struct chrdev {
    dev_t                 cd_dev;
    const char           *cd_name;
    const struct fileops *cd_fops;
    struct list_head      cd_linkage;
};

static LIST_HEAD(chrdevs);
static spinlock_t chrdevs_lock;

static struct chrdev *__find_chrdev(dev_t dev) {
    struct chrdev *cd;

    list_for_each_entry(cd, &chrdevs, cd_linkage) {
        if (cd->cd_dev == dev) { return cd; }
    }

    return NULL;
}

int chrdev_register(dev_t dev, const char *name, const struct fileops *fops) {
    struct chrdev *cd  = kmalloc(sizeof(struct chrdev), M_SLEEPOK);
    int            res = 0;

    if (!cd) { return -ENOMEM; }

    cd->cd_dev  = dev;
    cd->cd_name = name;
    cd->cd_fops = fops;
    list_init(&cd->cd_linkage);

    spin_lock(&chrdevs_lock);
    if (__find_chrdev(dev)) {
        res = -EEXIST;
    } else {
        list_add_tail(&cd->cd_linkage, &chrdevs);
    }
    spin_unlock(&chrdevs_lock);

    if (res) { kfree(cd); }
    return res;
}

int chrdev_unregister(dev_t dev) {
    struct chrdev *cd;

    spin_lock(&chrdevs_lock);
    cd = __find_chrdev(dev);
    if (cd) { list_del(&cd->cd_linkage); }
    spin_unlock(&chrdevs_lock);

    if (!cd) { return -ENOENT; }

    kfree(cd);
    return 0;
}

static int chrdev_open(struct inode *in, struct file *fp) {
    const struct fileops *fops = NULL;
    struct chrdev        *cd;

    spin_lock(&chrdevs_lock);
    cd = __find_chrdev(in->i_rdev);
    if (cd) { fops = cd->cd_fops; }
    spin_unlock(&chrdevs_lock);

    if (!fops) { return -ENXIO; }

    fp->f_op = fops;
    if (fops->fop_open) { return fops->fop_open(in, fp); }

    return 0;
}

const struct fileops chrdev_fops = {
        .fop_open = chrdev_open,
};
