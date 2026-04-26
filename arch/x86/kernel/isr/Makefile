O_OBJS := isr_common.o isr_dispatch.o generated/isr_entry.o

O_TARGET := kernelisrar.o

EXTRA_CFLAGS := -mgeneral-regs-only

include $(TOPDIR)/Rules.mk

generated/isr_entry.S: isr_vectors.json
	@echo -e "PY $@"
	$(Q)mkdir -p generated
	$(Q)$(PYTHON) $(TOPDIR)/scripts/isr_gen.py > $@
