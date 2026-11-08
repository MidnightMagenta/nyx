SRCDIR := $(patsubst %/,%,$(dir $(abspath $(firstword $(MAKEFILE_LIST)))))

vpath %.c $(SRCDIR)
vpath %.S $(SRCDIR)

P_OBJS := head.o init.o region_alloc_init.o

mb2ar.o: $(P_OBJS)
	@echo -e "LD $@"
	$(Q)$(LD) -r -o $@ $^

%.o: %.c
	@echo -e "CC $<"
	$(Q)$(CC) $(CFLAGS) -mgeneral-regs-only -MMD -MP -c $< -o $@

%.o: %.S
	@echo -e "AS $@"
	$(Q)$(CC) -x assembler-with-cpp $(ASFLAGS) -D__ASSEMBLY__ -MMD -MP -c $< -o $@

-include $(P_OBJS:.o=.d)
