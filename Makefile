MAKEFLAGS += --no-print-directory --no-builtin-rules

TOPDIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BUILD     ?= build
PYTHON    ?= python3
NINJA     ?= ninja
NINJAARGS ?=

MBUILD_DIR := tools/mbuild
MBUILD     := $(MBUILD_DIR)/mbuild
KCONFIGLIB := contrib/kconfiglib

ARCH          ?= $(shell cat .arch 2>/dev/null)
CROSS_COMPILE ?= $(shell cat .cross_compile 2>/dev/null)
export ARCH CROSS_COMPILE

PROTOCOL := $(shell sed -n 's/^CONFIG_BOOT_PROTOCOL="\(.*\)"$$/\1/p' .config 2>/dev/null)

PACKAGE_SCRIPT := tools/scripts/package/$(ARCH)-$(PROTOCOL).sh
PACKAGE_OUT    ?= $(BUILD)/nyxos.iso
export PACKAGE_OUT

NINJA_TARGETS := vmnyx image initramfs sbin

ifeq ($(abspath $(BUILD)),$(TOPDIR))
$(error BUILD must not be the source directory)
endif

.PHONY: all build mbuild config menuconfig package clean distclean check-arch FORCE $(NINJA_TARGETS) \
	ninjaclean compdb

all: build

check-arch:
	@test -n "$(ARCH)" || { echo "error: ARCH is not set and no .arch cache found" >&2; exit 1; }

# --------------------------------
# configuration
# --------------------------------

config: check-arch
	$(PYTHON) $(KCONFIGLIB)/oldconfig.py Kconfig

menuconfig: check-arch
	$(PYTHON) $(KCONFIGLIB)/menuconfig.py Kconfig

.config:
	@echo "error: no .config found, run 'make config' or 'make menuconfig' first" >&2
	@exit 1

compile_commands.json: $(BUILD)/build.ninja
	$(NINJA) -C $(BUILD) -t compdb > $@

compdb: compile_commands.json

# --------------------------------
# build
# --------------------------------

$(MBUILD): FORCE
	$(MAKE) -s -C $(MBUILD_DIR) all

mbuild: $(MBUILD)

$(BUILD)/build.ninja: $(MBUILD) .config | check-arch
	@mkdir -p $(BUILD)
	$(MBUILD) -C $(BUILD) $(TOPDIR)/Mbuild

build: $(BUILD)/build.ninja
	$(NINJA) -C $(BUILD) $(NINJAARGS)

$(NINJA_TARGETS): $(BUILD)/build.ninja
	$(NINJA) -C $(BUILD) $(NINJAARGS) $@

# --------------------------------
# packaging
# --------------------------------

package: $(BUILD)/build.ninja | check-arch
	@test -x $(PACKAGE_SCRIPT) || { echo "error: no packaging script $(PACKAGE_SCRIPT)" >&2; exit 1; }
	$(NINJA) -C $(BUILD) $(NINJAARGS) image initramfs
	$(PACKAGE_SCRIPT) $(BUILD)/sys/image $(BUILD)/initramfs.cpio

# --------------------------------
# cleanup
# --------------------------------

ninjaclean: $(BUILD)/build.ninja
	$(NINJA) -C $(BUILD) -t clean

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -f .config .config.old .arch .cross_compile compile_commands.json

FORCE:
