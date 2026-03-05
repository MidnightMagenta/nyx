.EXPORT_ALL_VARIABLES:

ifeq ($(V),y)
Q := 
else
Q := @
endif

MAKEFLAGS += --no-print-directory --no-builtin-rules

ARCH := x86
# HACK: CROSS_COMPILE should be empty (set as enviromental variable). Left filled for convenience right now
CROSS_COMPILE := x86_64-elf-
TOPDIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# --------------------------------
# toolchain
# --------------------------------

INCDIR := $(TOPDIR)/include

HOSTCC  := gcc
HOSTCXX := g++
HOSTAR  := ar

CC  := $(CROSS_COMPILE)gcc -I$(INCDIR)
CPP := $(CC) -E
AS  := $(CROSS_COMPILE)as
AR  := $(CROSS_COMPILE)ar
LD  := $(CROSS_COMPILE)ld

# --------------------------------
# flags
# --------------------------------

HOSTCFLAGS= -O2 -g
HOSTCXXFLAGS= -O2 -g

CFLAGS := -nostartfiles \
		  -nodefaultlibs \
		  -nostdlib \
		  -nostdinc \
		  -ffreestanding \
		  -fshort-wchar \
		  -fno-omit-frame-pointer \
		  -fno-stack-protector \
		  -fno-builtin \
		  -fno-pic -fno-pie \
		  -std=gnu23 \
		  -Wall -Wextra
CPPFLAGS :=
ASFLAGS :=
ASPPFLAGS := -D__ASSEMBLY__
LDFLAGS := -static -Bsymbolic -nostdlib

# --------------------------------

ARCHIVES := nyx/nyxar.o
LIBS     := lib/libnyx.a

SUBDIRS := nyx lib

.PHONY: all do-all nyxos nyxsubdirs clean distclean symlinks menuconfig config docs tools

all: do-all

ifeq (.config,$(wildcard .config))
include .config
do-all: nyxos
else
do-all: config
endif

# --------------------------------
# Configuration
# --------------------------------

ifeq ($(CONFIG_DEBUG),y)
CFLAGS += -O0 -g -D__DEBUG
else
CFLAGS += -O2
endif

ifeq ($(CONFIG_EXTRA_WARNINGS),y)
CFLAGS += -Wconversion -Wsign-conversion -Wundef -Wcast-align \
          -Wshift-overflow -Wdouble-promotion -Wpedantic
endif

ifeq ($(CONFIG_WARNINGS_AS_ERRORS),y)
CFLAGS += -Werror
endif

CFLAGS += -include $(TOPDIR)/include/generated/autoconf.h

include arch/$(ARCH)/Makefile

# --------------------------------
# General rules for building the kernel
# --------------------------------

nyxos: nyxsubdirs $(ARCH_LINK)
	@echo -e "LD $@"
	$(Q)$(LD) $(LDFLAGS) \
		-T $(ARCH_LINK) \
		$(ARCHIVES) \
		$(LIBS) \
		-o $@

nyxsubdirs: include/generated/autoconf.h
	$(Q)set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done

tools:
	$(Q)$(MAKE) -C tools

# --------------------------------
# Cleanup rules
# --------------------------------

clean: archclean
	find . -type f ! -path './scripts/*' -name '*.[oasd]' -delete
	rm -rf isodir
	rm -f nyxos nyxos.iso

distclean: clean
	$(Q)$(MAKE) -C tools clean
	rm -f .config .config.old include/asi
	rm -rf .cache out include/generated scripts/Kconfig/__pycache__

# --------------------------------
# Rules for setting up the project
# --------------------------------

include/generated/autoconf.h: .config
	$(Q)mkdir -p include/generated
	$(Q)./scripts/Kconfig/genconfig.py --header-path include/generated/autoconf.h

symlinks:
	rm -f include/asi
	( cd include ; ln -s ../arch/$(ARCH)/include/asi asi ; \
		cd uapi ; ln -s ../../arch/$(ARCH)/include/uapi/asi asi )

menuconfig: symlinks
	$(Q)./scripts/Kconfig/menuconfig.py

config: symlinks
	$(Q)./scripts/Kconfig/oldconfig.py

docs:
	$(Q)mkdir -p out/docs
	$(Q)doxygen
