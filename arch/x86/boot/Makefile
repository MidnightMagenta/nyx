SRCDIR := $(patsubst %/,%,$(dir $(abspath $(firstword $(MAKEFILE_LIST)))))

vpath %.c  $(SRCDIR)
vpath %.S  $(SRCDIR)
vpath %.ld $(SRCDIR)

SYSTEM := $(OBJ)/vmnyx
I_OBJS := printb.o boot_utils.o region_alloc.o vmnyx.o

ifdef CONFIG_MULTIBOOT2
P_DIR := multiboot2
P_OBJ := multiboot2/mb2ar.o
endif

I_OBJS += $(P_OBJ)

.PHONY: image FORCE

image: boot_link.lds $(I_OBJS)
	$(Q)echo -e "LD nyxos"
	$(Q)$(LD) $(BIMAGE_FLAGS) \
		-T boot_link.lds \
		-o $@ $(filter-out boot_link.lds,$^)

vmnyx.o: $(SYSTEM)
	$(Q)echo -e "OBJCOPY $@"
	$(Q)$(OBJCOPY) -I binary -O elf64-x86-64 \
		--rename-section .data=.kernel_blob,alloc,load,readonly,data,contents $< $@

# FORCE: the sub-make decides what actually needs rebuilding
$(P_OBJ): FORCE
	$(Q)mkdir -p $(P_DIR)
	$(Q)$(MAKE) -C $(P_DIR) -f $(SRCDIR)/$(P_DIR)/Makefile

FORCE:

%.o: %.c
	@echo -e "CC $@"
	$(Q)$(CC) $(CFLAGS) -mgeneral-regs-only -MMD -MP -c $< -o $@

%.o: %.S
	@echo -e "AS $@"
	$(Q)$(CC) -x assembler-with-cpp $(ASFLAGS) -D__ASSEMBLY__ -MMD -MP -c $< -o $@

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include $(GENDIR)/generated/autoconf.h $< -o $@

-include $(I_OBJS:.o=.d)

unexport SYSTEM
unexport I_OBJS
