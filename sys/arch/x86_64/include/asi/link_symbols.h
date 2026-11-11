#ifndef _ASI_LINK_SYMBOLS_H
#define _ASI_LINK_SYMBOLS_H

extern char __image_start[];
extern char __image_end[];
extern char __kernel_start[];
extern char __kernel_end[];
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];
extern char __init_start[];
extern char __init_end[];

#ifdef CONFIG_KERNEL_TESTS
extern char __kernel_tests_start[];
extern char __kernel_tests_end[];
#endif

#define SYMBOL_OFFSET(sym) ((char *) (sym) - (char *) __image_start)

#endif
