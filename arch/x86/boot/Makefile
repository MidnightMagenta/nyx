ifeq ($(CONFIG_BOOT_MULTIBOOT2),y)
SUB_DIRS := Multiboot2
O_OBJS := Multiboot2/multiboot2.o
else
$(error Invalid boot type)
endif

O_TARGET := boot.o

include $(TOPDIR)/Rules.mk
