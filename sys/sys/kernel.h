#ifndef _NYX_KERNEL_H
#define _NYX_KERNEL_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define IS_POWER_OF_TWO(x) ((x) && !((x) & ((x) - 1)))

#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))

#include <nyx/printk.h>

enum {
    LOG_NONE  = 0,
    LOG_ERR   = 1,
    LOG_WARN  = 2,
    LOG_INFO  = 3,
    LOG_DEBUG = 4,
};

struct subsys_log {
    const char *name;
    int         level;
};

#define DECLARE_SUBSYS_LOG(varname) extern struct subsys_log *__##varname##_ptr
#define DEFINE_SUBSYS_LOG(varname, subsys_name, default_level)                                                         \
    static struct subsys_log varname = {.name = subsys_name, .level = default_level};                                  \
    struct subsys_log       *__##varname##_ptr __attribute__((section(".subsys_logs"), used)) = &varname

DECLARE_SUBSYS_LOG(log_generic);

#define __PR(req_level, subsys, f, ...)                                                                                \
    do {                                                                                                               \
        if (CONFIG_PRINTK_VERBOSITY >= req_level && (__##subsys##_ptr)->level >= req_level) {                          \
            printk("[%s]: " f, (__##subsys##_ptr)->name, ##__VA_ARGS__);                                               \
        }                                                                                                              \
    } while (0)

#define pr_error(subsys, f, ...) __PR(LOG_ERR, subsys, f, ##__VA_ARGS__)
#define pr_warn(subsys, f, ...)  __PR(LOG_WARN, subsys, f, ##__VA_ARGS__)
#define pr_info(subsys, f, ...)  __PR(LOG_INFO, subsys, f, ##__VA_ARGS__)
#define pr_debug(subsys, f, ...) __PR(LOG_DEBUG, subsys, f, ##__VA_ARGS__)

#endif
