#ifndef _UPAI_MMAN_H
#define _UPAI_MMAN_H

#define PROT_READ  (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC  (1 << 2)
#define PROT_NONE  (1 << 3)

#define MAP_PRIVATE   (1 << 0)
#define MAP_SHARED    (1 << 1)
#define MAP_ANONYMOUS (1 << 2)
#define MAP_ANON      MAP_ANONYMOUS
#define MAP_FIXED     (1 << 3)

#endif
