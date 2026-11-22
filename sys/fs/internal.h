#ifndef _NYX_FS_INTERNAL_H
#define _NYX_FS_INTERNAL_H

#include <sys/spinlock.h>
#include <sys/vfs.h>

extern spinlock_t mount_lock;

void dcache_init();
void inode_init();
void file_init();
void filedesc_init();

void follow_mounts(struct path *p);
void follow_dotdot(struct path *p, const struct path *root);
int  get_start_paths(const struct path *dir, const char *name, struct path *start, struct path *root);
int  vfs_open_path(struct path *p, int flags, struct file **out);

#endif
