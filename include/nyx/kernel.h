#ifndef _NYX_KERNEL_H
#define _NYX_KERNEL_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define IS_POWER_OF_TWO(x) ((x) && !((x) & ((x) - 1)))

#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))

#include <nyx/printk.h>

#define pr_error(f, ...) printk("error [%s:%d]: " f, __FILE__, __LINE__, ##__VA_ARGS__)
#define pr_warn(f, ...)  printk("warning: " f, ##__VA_ARGS__)
#define pr_info(f, ...)  printk("info: " f, ##__VA_ARGS__)
#ifdef __DEBUG
#define pr_dbg(f, ...) printk("debug [%s:%d]: " f, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define pr_dbg(f, ...)
#endif

#endif
