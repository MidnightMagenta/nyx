ARCHIVES += arch/$(ARCH)/boot/bootar.o \
			arch/$(ARCH)/init/initar.o \
			arch/$(ARCH)/kernel/kernelar.o
LIBS +=

SUBDIRS += arch/$(ARCH)/boot \
		   arch/$(ARCH)/init \
		   arch/$(ARCH)/kernel

ifdef CONFIG_X86_32
$(warning i386 is unsupported)
else ifdef CONFIG_X86_64
CFLAGS += -m64 -m80387 -msse -msse2 -mmmx \
          -mno-sse3 -mno-sse4 -mno-avx -mno-avx2 \
          -mno-avx512f -mcmodel=kernel \
          -mno-red-zone -fno-tree-vectorize
ASFLAGS += -m64
ARCH_LINK := $(TOPDIR)/arch/$(ARCH)/X86_64_link.lds
endif

.PHONY: archclean

archclean:
	rm -f arch/$(ARCH)/X86_64_link.lds

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include include/generated/autoconf.h $^ -o $@
