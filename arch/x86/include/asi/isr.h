#ifndef _ASI_ISR_H
#define _ASI_ISR_H

#include <asi/isr_context.h>

typedef void (*isr_func_t)(struct isr_context *);

int isr_register(int vector, isr_func_t fn);
int isr_deregister(int vector);

#endif
