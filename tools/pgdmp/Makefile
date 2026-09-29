SRCDIR := $(patsubst %/,%,$(dir $(abspath $(firstword $(MAKEFILE_LIST)))))

vpath %.c $(SRCDIR)

.PHONY: all

all: pgdmp_x86

pgdmp_x86: pgdmp_x86.o
	@echo -e "LD $@"
	@$(HOSTCC) -o $@ $^

%.o: %.c
	@echo -e "CC $@"
	@$(HOSTCC) $(HOSTCFLAGS) -MMD -MP -c -o $@ $<

-include pgdmp_x86.d
