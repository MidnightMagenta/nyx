TOOL_DIRS := pgdmp

.PHONY: all tools

all: tools

tools:
	$(Q)set -e; for i in $(TOOL_DIRS); do $(MAKE) -C $$i; done

clean:
	$(Q)set -e; for i in $(TOOL_DIRS); do $(MAKE) -C $$i clean; done
