ARCHIVES += arch/$(ARCH)/init/initar.o \
			arch/$(ARCH)/kernel/kernelar.o
LIBS +=

SUBDIRS += arch/$(ARCH)/init \
		   arch/$(ARCH)/kernel

CFLAGS += -m64 -m80387 -msse -msse2 -mmmx \
          -mno-sse3 -mno-sse4 -mno-avx -mno-avx2 \
          -mno-avx512f -mcmodel=kernel \
          -mno-red-zone -fno-tree-vectorize
ASFLAGS += -m64
ARCH_LINK := $(TOPDIR)/arch/$(ARCH)/X86_64_link.lds

.PHONY: archclean bImage

MAKEBOOT := $(MAKE) -C arch/$(ARCH)/boot

bImage: vmnyx
	$(MAKEBOOT) bImage

archclean:
	rm -f arch/$(ARCH)/X86_64_link.lds
	$(MAKEBOOT) clean

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include include/generated/autoconf.h $^ -o $@
