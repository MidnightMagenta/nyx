#ifndef _ASI_GENERIC_LIMITS_H
#define _ASI_GENERIC_LIMITS_H

#include <nyx/types.h>

#define __TYPE_MAX(t)        ((t) ~(t) 0)
#define __TYPE_SIGNED_MAX(t) ((t) (__TYPE_MAX(t) >> 1))
#define __TYPE_SIGNED_MIN(t) ((t) (-__TYPE_SIGNED_MAX(t) - 1))

#ifndef U8_MAX
#define U8_MAX __TYPE_MAX(u8)
#endif

#ifndef U16_MAX
#define U16_MAX __TYPE_MAX(u16)
#endif

#ifndef U32_MAX
#define U32_MAX __TYPE_MAX(u32)
#endif

#ifndef U64_MAX
#define U64_MAX __TYPE_MAX(u64)
#endif

#ifndef S8_MAX
#define S8_MAX __TYPE_SIGNED_MAX(s8)
#endif

#ifndef S8_MIN
#define S8_MIN __TYPE_SIGNED_MIN(s8)
#endif

#ifndef S16_MAX
#define S16_MAX __TYPE_SIGNED_MAX(s16)
#endif

#ifndef S16_MIN
#define S16_MIN __TYPE_SIGNED_MIN(s16)
#endif

#ifndef S32_MAX
#define S32_MAX __TYPE_SIGNED_MAX(s32)
#endif

#ifndef S32_MIN
#define S32_MIN __TYPE_SIGNED_MIN(s32)
#endif

#ifndef S64_MAX
#define S64_MAX __TYPE_SIGNED_MAX(s64)
#endif

#ifndef S64_MIN
#define S64_MIN __TYPE_SIGNED_MIN(s64)
#endif

#ifndef PHYS_ADDR_MAX
#define PHYS_ADDR_MAX __TYPE_MAX(phys_addr_t)
#endif

#ifndef VIRT_ADDR_MAX
#define VIRT_ADDR_MAX __TYPE_MAX(virt_addr_t)
#endif

#endif
