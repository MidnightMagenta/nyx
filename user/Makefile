SRCDIR := $(patsubst %/,%,$(dir $(abspath $(firstword $(MAKEFILE_LIST)))))
vpath %.c $(SRCDIR)/bin $(SRCDIR)/libc $(SRCDIR)
vpath %.S $(SRCDIR)/crt

UCC := $(CROSS_COMPILE)gcc
ULD := $(CROSS_COMPILE)ld
UCFLAGS := -std=gnu23 -O2 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
           -mno-sse -mno-mmx -msoft-float -Wall -Wextra \
           -I$(TOPDIR)/include/uapi -I$(SRCDIR)/libc/include
ULDFLAGS := -static -nostdlib -T $(SRCDIR)/_user.ld

PROGS := init

init_OBJS := init.o

all: $(PROGS)

install: all
	$(Q)mkdir -p $(DESTDIR)/bin
	$(Q)for p in $(PROGS); do \
		cp $$p $(DESTDIR)/bin/$$p; \
	done

%.o: %.c
	@echo -e "UCC $@"
	$(Q)$(UCC) $(UCFLAGS) -MMD -MP -c $< -o $@

%.o: %.S
	@echo -e "UAS $@"
	$(Q)$(UCC) -x assembler-with-cpp $(UCFLAGS) -MMD -MP -c $< -o $@

define PROG_RULE
$(1): $$($(1)_OBJS) $(SRCDIR)/_user.ld
	@echo -e "ULD $$@"
	$(Q)$(ULD) $(ULDFLAGS) -o $$@ $$($(1)_OBJS)
endef

$(foreach p,$(PROGS),$(eval $(call PROG_RULE,$(p))))

-include $(wildcard *.d)
