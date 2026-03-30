#ifndef _NYX_CONTAINER_OF_H
#define _NYX_CONTAINER_OF_H

#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))

#endif
