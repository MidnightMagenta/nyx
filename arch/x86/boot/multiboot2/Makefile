P_OBJS := head.o init.o region_alloc_init.o

.PHONY: clean

mb2ar.o: $(P_OBJS)
	@echo -e "LD $@"
	$(Q)$(LD) -r -o $@ $^

clean:

%.o: %.c
	@echo -e "CC $<"
	$(Q)$(CC) $(CFLAGS) -mgeneral-regs-only -MMD -MP -c $< -o $@

%.o: %.S
	@echo -e "AS $@"
	$(Q)$(CC) -x assembler-with-cpp $(ASFLAGS) -D__ASSEMBLY__ -MMD -MP -c $< -o $@

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include $(TOPDIR)/include/generated/autoconf.h $^ -o $@

-include $(P_OBJS:.o=.d)
