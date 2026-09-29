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
ARCH_LINK := $(OBJ)/arch/$(ARCH)/X86_64_link.lds

PHONY += image archtargets

MAKEBOOT := $(MAKE) -C $(OBJ)/arch/$(ARCH)/boot -f $(TOPDIR)/arch/$(ARCH)/boot/Makefile

kimage: vmnyx
	$(Q)mkdir -p $(OBJ)/arch/$(ARCH)/boot
	$(Q)$(MAKEBOOT) image
	$(Q)cp $(OBJ)/arch/$(ARCH)/boot/image $(OBJ)/image

ASM_OFFSETS_S := $(OBJ)/arch/$(ARCH)/kernel/asm-offsets.s
ASM_OFFSETS_H := $(GENDIR)/asi/generated/asm-offsets.h

# special targets that arch needs to be built before the kernel
archtargets: $(ASM_OFFSETS_H)

$(ARCH_LINK): $(TOPDIR)/arch/$(ARCH)/X86_64_link.ld $(GENDIR)/generated/autoconf.h
	@echo -e "CPP $@"
	$(Q)mkdir -p $(@D)
	$(Q)$(CPP) -P -x assembler-with-cpp -include $(GENDIR)/generated/autoconf.h $< -o $@

$(ASM_OFFSETS_S): $(TOPDIR)/arch/$(ARCH)/kernel/asm-offsets.c $(GENDIR)/generated/autoconf.h
	@echo "CC $@"
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(CFLAGS) -MMD -MP -S $< -o $@

$(ASM_OFFSETS_H): $(ASM_OFFSETS_S)
	@echo "SED $@"
	@mkdir -p $(@D)
	$(Q)grep "^->" $< | sed 's/^->\([^ ]*\) \$$\?\(.*\)/#define \1 \2/' > $@

-include $(ASM_OFFSETS_S:.s=.d)
