ifdef CONFIG_X86_32
$(error i386 is unsupported)
O_OBJS += startup32.o
else ifdef CONFIG_X86_64
O_OBJS += startup64.o
endif

O_OBJS += init.o

O_TARGET := bootar.o

include $(TOPDIR)/Rules.mk
