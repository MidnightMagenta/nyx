ARCHIVES += arch/$(ARCH)/kernel/kernelar.o arch/$(ARCH)/mm/mmar.o \
			arch/$(ARCH)/lib/archliba.o
LIBS += 

SUBDIRS += arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/lib

CFLAGS += -m64 -m80387 -mno-sse -mno-sse2 -mno-mmx \
          -mno-sse3 -mno-sse4 -mno-avx -mno-avx2 \
          -mno-avx512f -msoft-float -mcmodel=kernel \
          -mno-red-zone -fno-tree-vectorize
ASFLAGS += -m64
ARCH_LINK := $(TOPDIR)/arch/$(ARCH)/X86_64_link.lds

PHONY += image archclean

MAKEBOOT := $(MAKE) -C arch/$(ARCH)/boot

image: vmnyx
	$(Q)$(MAKEBOOT) image
	$(Q)cp arch/$(ARCH)/boot/image image

archclean:
	rm -f arch/$(ARCH)/X86_64_link.lds
	rm -rf arch/$(ARCH)/kernel/isr/generated
	$(MAKEBOOT) clean

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include include/generated/autoconf.h $^ -o $@
