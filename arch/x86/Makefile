ARCHIVES += arch/$(ARCH)/kernel/kernelar.o arch/$(ARCH)/mm/mmar.o \
			arch/$(ARCH)/lib/archliba.o arch/$(ARCH)/entry/entryar.o
LIBS += 

SUBDIRS += arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/lib \
		   arch/$(ARCH)/entry

CFLAGS += -m64 -m80387 -mno-sse -mno-sse2 -mno-mmx \
          -mno-sse3 -mno-sse4 -mno-avx -mno-avx2 \
          -mno-avx512f -msoft-float -mcmodel=kernel \
          -mno-red-zone -fno-tree-vectorize
ASFLAGS += -m64
ARCH_LINK := $(TOPDIR)/arch/$(ARCH)/X86_64_link.lds

PHONY += image archclean archtargets

MAKEBOOT := $(MAKE) -C arch/$(ARCH)/boot

image: vmnyx
	$(Q)$(MAKEBOOT) image
	$(Q)cp arch/$(ARCH)/boot/image image

# special targets that arch needs to be built before the kernel
archtargets: $(TOPDIR)/arch/$(ARCH)/include/asi/generated/asm-offsets.h

archclean:
	rm -f arch/$(ARCH)/X86_64_link.lds
	$(MAKEBOOT) clean

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include include/generated/autoconf.h $^ -o $@

$(TOPDIR)/arch/$(ARCH)/kernel/asm-offsets.s: $(TOPDIR)/arch/$(ARCH)/kernel/asm-offsets.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -S $< -o $@

$(TOPDIR)/arch/$(ARCH)/include/asi/generated/asm-offsets.h: $(TOPDIR)/arch/$(ARCH)/kernel/asm-offsets.s
	@echo "SED $@"
	@mkdir -p $(@D)
	$(Q)grep "^->" $< | sed 's/^->\([^ ]*\) \$$\?\(.*\)/#define \1 \2/' > $@

-include $(TOPDIR)/arch/$(ARCH)/kernel/asm-offsets.d
