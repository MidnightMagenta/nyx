#include <sys/kernel.h>
#include <sys/list.h>
#include <sys/mm/mm_types.h>
#include <sys/mm/slab.h>
#include <sys/string.h>
#include <sys/vfs.h>

#include <asi/bug.h>

#include "internal.h"

static kmem_cache_t *dentry_cache;

void dcache_init() {
    dentry_cache = kmem_create_cache("dentry", sizeof(struct dentry), _Alignof(struct dentry), NULL, NULL, M_SLEEPOK);
    BUG_ON(!dentry_cache);
}

static struct dentry *__dalloc(const char *name, size_t namelen) {
    struct dentry *dent;

    if (namelen > NAME_MAX) { return NULL; }

    dent = kmem_cache_alloc(dentry_cache, M_SLEEPOK);
    if (!dent) { return NULL; }

    memset(dent, 0, sizeof(struct dentry));
    memcpy(dent->d_name, name, namelen);
    dent->d_name[namelen] = '\0';
    dent->d_namelen       = namelen;
    refcount_init(&dent->d_refs, 1);
    spinlock_init(&dent->d_lock);
    list_init(&dent->d_children);
    list_init(&dent->d_siblings);

    return dent;
}

struct dentry *dalloc(struct dentry *parent, const char *name, size_t namelen) {
    struct dentry *dent = __dalloc(name, namelen);
    if (!dent) { return NULL; }

    dget(parent);
    dent->d_parent = parent;
    dent->d_sb     = parent->d_sb;

    spin_lock(&parent->d_lock);
    list_add_tail(&dent->d_siblings, &parent->d_children);
    spin_unlock(&parent->d_lock);

    return dent;
}

struct dentry *dalloc_root(struct superblock *sb, struct inode *root) {
    struct dentry *dent = __dalloc("/", 1);
    if (!dent) { return NULL; }

    dent->d_parent = dent;
    dent->d_sb     = sb;
    dent->d_inode  = root;

    return dent;
}

struct dentry *dlookup(struct dentry *parent, const char *name, size_t namelen) {
    struct dentry *child;
    struct dentry *found = NULL;

    spin_lock(&parent->d_lock);
    list_for_each_entry(child, &parent->d_children, d_siblings) {
        if (child->d_namelen == namelen && !memcmp(child->d_name, name, namelen)) {
            refcount_inc(&child->d_refs);
            found = child;
            break;
        }
    }
    spin_unlock(&parent->d_lock);

    return found;
}

void dget(struct dentry *dent) {
    BUG_ON(refcount_get(&dent->d_refs) == 0);
    refcount_inc(&dent->d_refs);
}

static void dfree(struct dentry *dent) {
    if (dent->d_inode) { iput(dent->d_inode); }
    kmem_cache_free(dentry_cache, dent);
}

void dput(struct dentry *dent) {
    while (dent) {
        struct dentry *parent = dent->d_parent;

        if (parent == dent) {
            if (refcount_dec_and_test(&dent->d_refs)) { dfree(dent); }
            return;
        }

        spin_lock(&parent->d_lock);
        if (!refcount_dec_and_test(&dent->d_refs)) {
            spin_unlock(&parent->d_lock);
            return;
        }
        if (!(dent->d_flags & D_DEAD) && dent->d_inode) {
            spin_unlock(&parent->d_lock);
            return;
        }
        if (!(dent->d_flags & D_DEAD)) {
            list_del(&dent->d_siblings);
            dent->d_flags |= D_DEAD;
        }
        spin_unlock(&parent->d_lock);

        dfree(dent);
        dent = parent;
    }
}

void dinstantiate(struct dentry *dent, struct inode *inode) {
    spin_lock(&dent->d_lock);
    BUG_ON(dent->d_inode);
    dent->d_inode = inode;
    spin_unlock(&dent->d_lock);
}

void dunlink(struct dentry *dent) {
    struct dentry *parent = dent->d_parent;

    BUG_ON(parent == dent);

    spin_lock(&parent->d_lock);
    if (!(dent->d_flags & D_DEAD)) {
        list_del(&dent->d_siblings);
        dent->d_flags |= D_DEAD;
    }
    spin_unlock(&parent->d_lock);
}

void dshrink(struct dentry *root, int held) {
    struct dentry *dent = root;

    for (;;) {
        struct dentry *child = NULL;
        struct dentry *parent;

        spin_lock(&dent->d_lock);
        if (!list_is_empty(&dent->d_children)) {
            child = list_first_entry(&dent->d_children, struct dentry, d_siblings);
        }
        spin_unlock(&dent->d_lock);

        if (child) {
            dent = child;
            continue;
        }
        if (dent == root) { return; }

        parent = dent->d_parent;
        BUG_ON(refcount_get(&dent->d_refs) != held);

        spin_lock(&parent->d_lock);
        list_del(&dent->d_siblings);
        dent->d_flags |= D_DEAD;
        spin_unlock(&parent->d_lock);

        dfree(dent);
        refcount_dec(&parent->d_refs);
        dent = parent;
    }
}
