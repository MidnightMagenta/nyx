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

# Kconfig strings are quoted in .config
PROTOCOL := $(shell sed -n 's/^CONFIG_BOOT_PROTOCOL="\(.*\)"$$/\1/p' .config 2>/dev/null)

PACKAGE_SCRIPT := tools/scripts/package/$(ARCH)-$(PROTOCOL).sh
PACKAGE_OUT    ?= $(BUILD)/nyxos.iso
export PACKAGE_OUT

NINJA_TARGETS := vmnyx image initramfs sbin

ifeq ($(abspath $(BUILD)),$(TOPDIR))
$(error BUILD must not be the source directory)
endif

.PHONY: all build mbuild config menuconfig package clean distclean check-arch FORCE $(NINJA_TARGETS)

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

# --------------------------------
# build
# --------------------------------

# the sub-make decides whether mbuild actually needs rebuilding
$(MBUILD): FORCE
	$(MAKE) -C $(MBUILD_DIR) all

mbuild: $(MBUILD)

# once generated, ninja regenerates build.ninja itself when the Mbuild scripts change
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

# one ninja invocation, so -j doesn't run two ninjas on the same build dir
package: $(BUILD)/build.ninja | check-arch
	@test -x $(PACKAGE_SCRIPT) || { echo "error: no packaging script $(PACKAGE_SCRIPT)" >&2; exit 1; }
	$(NINJA) -C $(BUILD) $(NINJAARGS) image initramfs
	$(PACKAGE_SCRIPT) $(BUILD)/sys/image $(BUILD)/initramfs.cpio

# --------------------------------
# cleanup
# --------------------------------

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -f .config .config.old .arch .cross_compile

FORCE:
