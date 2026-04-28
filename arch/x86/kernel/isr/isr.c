#include <nyx/errno.h>
#include <nyx/stddef.h>

#include <asi/idt.h>
#include <asi/isr.h>

isr_func_t isr_fn_arr[IDT_ENTRIES] = {0};

int isr_register(int vector, isr_func_t fn) {
    if ((unsigned int) vector > IDT_ENTRIES) { return -EINVAL; }
    if (isr_fn_arr[vector]) { return -EBUSY; }
    isr_fn_arr[vector] = fn;
    return 0;
}

int isr_deregister(int vector) {
    if ((unsigned int) vector > IDT_ENTRIES) { return -EINVAL; }
    isr_fn_arr[vector] = NULL;
    return 0;
}
