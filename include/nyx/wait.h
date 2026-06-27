#ifndef _NYX_WAIT_H
#define _NYX_WAIT_H

#include <nyx/proc.h>

void unsleep(struct thread *t);
void sleep_setup(const volatile void *ident, const char *wmesg);
void sleep_finish(int do_sleep);
void wakeup(const volatile void *ident);
void wakeup_one(const volatile void *ident);

#endif
