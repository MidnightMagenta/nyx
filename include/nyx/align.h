#ifndef _NYX_ALIGN_H
#define _NYX_ALIGN_H

#define ALIGN_UP(v, a)   (((v) + ((a) - 1)) & (~((a) - 1)))
#define ALIGN_DOWN(v, a) ((v) & ~((a) - 1))

#endif
