SYSTEM := $(TOPDIR)/vmnyx
I_OBJS := head.o init.o printb.o boot_utils.o region_alloc.o vmnyx.o

.PHONY: image clean

image: boot_link.lds $(I_OBJS)
	$(Q)echo -e "LD nyxos"
	$(Q)$(LD) $(BIMAGE_FLAGS) \
		-T boot_link.lds \
		-o $@ $(filter-out boot_link.lds,$^)

vmnyx.o: $(SYSTEM)
	$(Q)echo -e "OBJCOPY $@"
	$(Q)$(OBJCOPY) -I binary -O elf64-x86-64 \
		--rename-section .data=.kernel_blob,alloc,load,readonly,data,contents $< $@

clean:
	rm -f image vmnyx.o boot_link.lds

%.o: %.c
	@echo -e "CC $<"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.o: %.S
	@echo -e "AS $@"
	$(Q)$(CC) -x assembler-with-cpp $(ASFLAGS) -D__ASSEMBLY__ -MMD -MP -c $< -o $@

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include $(TOPDIR)/include/generated/autoconf.h $^ -o $@

-include $(I_OBJS:.o=.d)
