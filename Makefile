.EXPORT_ALL_VARIABLES:

ifeq ($(V),y)
Q := 
else
Q := @
endif

MAKEFLAGS += --no-print-directory --no-builtin-rules
SHELL := bash

ARCH := x86
# HACK: minor, build - CROSS_COMPILE should be empty (set as enviromental variable). Left filled for convenience right now
CROSS_COMPILE := x86_64-elf-
TOPDIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

O   ?= build
OBJ := $(abspath $(O))

ifeq ($(OBJ),$(TOPDIR))
$(error O must not be the source directory)
endif

# --------------------------------
# toolchain
# --------------------------------

INCDIR := $(TOPDIR)/include
GENDIR := $(OBJ)/include

HOSTCC  := gcc
HOSTCXX := g++
HOSTAR  := ar

CC  := $(CROSS_COMPILE)gcc -I$(GENDIR) -I$(INCDIR)
CPP := $(CC) -E
AS  := $(CROSS_COMPILE)as
AR  := $(CROSS_COMPILE)ar
LD  := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
PYTHON := python3
SH := bash

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
		  -fmacro-prefix-map=$(TOPDIR)/= \
		  -Wall -Wextra
CPPFLAGS := -D__KERNEL__
ASFLAGS :=
ASPPFLAGS := -D__ASSEMBLY__
LDFLAGS := -static -Bsymbolic -nostdlib

# --------------------------------

ARCHIVES := init/initar.o kernel/kernelar.o \
			mm/mmar.o lib/nyxliba.o fs/fsar.o \
			dev/drvar.o
LIBS     :=

SUBDIRS := init kernel mm lib fs dev

PHONY := all do-all vmnyx nyxsubdirs clean distclean symlinks menuconfig config docs tools

all: do-all

ifeq (.config,$(wildcard .config))
include .config
do-all: vmnyx
else
do-all: config
endif

# --------------------------------
# Configuration
# --------------------------------

ifdef CONFIG_DEBUG
CFLAGS += -O0 -g -D__DEBUG
else
CFLAGS += -O2
endif

ifdef CONFIG_KERNEL_TESTS
SUBDIRS += tests
ARCHIVES += tests/kerneltestsar.o
endif

ifdef CONFIG_EXTRA_WARNINGS
CFLAGS += -Wconversion -Wsign-conversion -Wundef -Wcast-align \
          -Wshift-overflow -Wdouble-promotion -Wpedantic
endif

ifdef CONFIG_WARNINGS_AS_ERRORS
CFLAGS += -Werror
endif

CFLAGS += -include $(GENDIR)/generated/autoconf.h
CFLAGS += -include $(GENDIR)/generated/version.h

include arch/$(ARCH)/Makefile

# --------------------------------
# General rules for building the kernel
# --------------------------------

PHONY += vmnyx nyxsubdirs tools

vmnyx: nyxsubdirs $(ARCH_LINK)
	@echo -e "LD $@"
	$(Q)$(LD) $(LDFLAGS) \
		-T $(ARCH_LINK) \
		$(addprefix $(OBJ)/,$(ARCHIVES)) \
		$(LIBS) \
		-o $(OBJ)/vmnyx

nyxsubdirs: $(GENDIR)/generated/autoconf.h $(GENDIR)/generated/version.h archtargets
	$(Q)set -e; for i in $(SUBDIRS); do \
		mkdir -p $(OBJ)/$$i; \
		$(MAKE) -C $(OBJ)/$$i -f $(TOPDIR)/$$i/Makefile; \
	done

tools:
	$(Q)mkdir -p $(OBJ)/tools
	$(Q)$(MAKE) -C $(OBJ)/tools -f $(TOPDIR)/tools/Makefile

# --------------------------------
# Cleanup rules
# --------------------------------

PHONY += clean distclean

clean:
	rm -rf $(OBJ)
	rm -rf isodir
	rm -f nyxos.iso

PHONY += srcclean
srcclean:
	find . -type f ! -path './scripts/*' ! -path './.git/*' -name '*.[oasd]' -delete
	find . -type d ! -path './.git/*' -name "generated" -prune -exec rm -rf {} +
	rm -f arch/$(ARCH)/X86_64_link.lds arch/$(ARCH)/boot/boot_link.lds
	rm -f vmnyx image

distclean: clean
	find . -type d -name "tmp" -prune -exec rm -rf {} +
	rm -f .config .config.old include/asi
	rm -rf .cache out scripts/Kconfig/__pycache__

# --------------------------------
# Rules for setting up the project
# --------------------------------

$(GENDIR)/generated/version.h: $(GENDIR)/generated/version.h.tmp
	@if ! cmp -s $< $@; then cp $< $@; fi

$(GENDIR)/generated/version.h.tmp: FORCE
	$(Q)mkdir -p $(@D)
	$(Q)rm -f $@
	$(Q)$(SH) ./scripts/mkversion.sh $@

$(GENDIR)/generated/autoconf.h: .config
	$(Q)mkdir -p $(@D)
	$(Q)$(PYTHON) ./scripts/Kconfig/genconfig.py --header-path $@

PHONY += symlinks menuconfig config docs

symlinks:
	rm -f include/asi include/uapi/asi
	( cd include ; ln -s ../arch/$(ARCH)/include/asi asi ; \
		cd uapi ; ln -s ../../arch/$(ARCH)/include/uapi/asi asi )

menuconfig: symlinks
	$(Q)$(PYTHON) ./scripts/Kconfig/menuconfig.py

config: symlinks
	$(Q)$(PYTHON) ./scripts/Kconfig/oldconfig.py

docs:
	$(Q)mkdir -p out/docs
	$(Q)doxygen

FORCE:

.PHONY: $(PHONY)
