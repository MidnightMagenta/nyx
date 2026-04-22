#ifndef _NYX_KERNEL_H
#define _NYX_KERNEL_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define IS_POWER_OF_TWO(x) ((x) && !((x) & ((x) - 1)))

#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))

#include <nyx/printk.h>

#define __PR(level, f, ...)                                                                                            \
    do {                                                                                                               \
        if (CONFIG_PRINTK_VERBOSITY >= (level)) { printk(f, ##__VA_ARGS__); }                                          \
    } while (0)

#define pr_error(f, ...)   __PR(1, "error [%s:%d]: " f, __FILE__, __LINE__, ##__VA_ARGS__)
#define pr_warn(f, ...)    __PR(2, "warning: " f, ##__VA_ARGS__)
#define pr_info(f, ...)    __PR(3, "info: " f, ##__VA_ARGS__)
#define pr_verbose(f, ...) __PR(4, "verbose: " f, ##__VA_ARGS__);

#ifdef __DEBUG
#define pr_dbg(f, ...) printk("debug [%s:%d]: " f, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define pr_dbg(f, ...)
#endif

#endif
