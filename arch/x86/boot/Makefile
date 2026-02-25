ifeq ($(CONFIG_BOOT_MULTIBOOT2),y)
SUB_DIRS := multiboot2
O_OBJS := multiboot2/multiboot2.o
else
$(error Invalid boot type)
endif

ifdef CONFIG_X86_32
$(error i386 is unsupported)
O_OBJS += startup32.o
else ifdef CONFIG_X86_64
O_OBJS += startup64.o
endif

O_TARGET := boot.o

include $(TOPDIR)/Rules.mk
