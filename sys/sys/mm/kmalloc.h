#ifndef _MM_KMALLOC_H
#define _MM_KMALLOC_H

void *kmalloc(unsigned long size, int flags);
void  kfree(void *addr);

#endif
