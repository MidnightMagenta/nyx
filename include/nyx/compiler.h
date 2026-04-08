#ifndef _NYX_COMPILER_H
#define _NYX_COMPILER_H

#define __packed        __attribute__((packed))
#define __align(a)      __attribute__((aligned(a)))
#define __section(sect) __attribute__((section(sect)))

#define __noreturn __attribute__((noreturn))

#if __has_attribute(__externally_visible__)
#define __visible __attribute__((__externally_visible__))
#else
#define __visible
#endif

#define __unused      __attribute__((unused))
#define __unused_p(p) (void) p
#endif

#define __weak          __attribute__((weak))
#define __weak_alias(a) __attribute__((weak, alias(a)))
