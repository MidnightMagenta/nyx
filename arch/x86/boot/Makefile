ifeq ($(CONFIG_BOOT_MULTIBOOT2),y)
SUB_DIRS := Multiboot2
O_OBJS := Multiboot2/multiboot2.o
else
$(error Invalid boot type)
endif

ifdef CONFIG_X86_32
$(warning i386 is unsupported)
else ifdef CONFIG_X86_64
O_OBJS += startup64.o
endif

O_TARGET := boot.o

include $(TOPDIR)/Rules.mk
