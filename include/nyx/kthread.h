#ifndef _NYX_KTHREAD_H
#define _NYX_KTHREAD_H

int  kthread_create(void (*entry)(void));
void kthread_exit();

#endif
