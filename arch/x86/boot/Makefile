SYSTEM := $(TOPDIR)/vmnyx

.PHONY: bImage clean

bImage: head.o init.o printb.o vmnyx.o | boot_link.lds
	$(Q)echo -e "LD nyxos"
	$(Q)$(LD) $(BIMAGE_FLAGS) \
		-T boot_link.lds \
		$^ -o $(TOPDIR)/nyxos

vmnyx.o: $(SYSTEM)
	$(Q)echo -e "OBJCOPY $@"
	$(Q)$(OBJCOPY) -I binary -O elf64-x86-64 \
		--rename-section .data=.kernel_blob,alloc,load,readonly,data,contents $< $@

clean:
	rm -f nyxos.o boot_link.lds

%.o: %.c
	@echo -e "CC $<"
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

%.o: %.S
	@echo -e "AS $@"
	$(Q)$(CC) -x assembler-with-cpp $(ASFLAGS) $(ASPPFLAGS) -MMD -MP -c $< -o $@

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include $(TOPDIR)/include/generated/autoconf.h $^ -o $@

-include head.d init.d
