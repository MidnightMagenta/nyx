SRCDIR := $(patsubst %/,%,$(dir $(abspath $(firstword $(MAKEFILE_LIST)))))

TOOL_DIRS := pgdmp

.PHONY: all tools

all: tools

tools:
	$(Q)set -e; for i in $(TOOL_DIRS); do \
		mkdir -p $$i; \
		$(MAKE) -C $$i -f $(SRCDIR)/$$i/Makefile; \
	done
