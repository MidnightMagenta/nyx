ARCHIVES += arch/$(ARCH)/boot/boot.o
LIBS     +=

SUBDIRS  += arch/$(ARCH)/boot

ifeq ($(CONFIG_X86_32),y)
$(error i386 is unsupported)
else ifeq ($(CONFIG_X86_64),y)
CFLAGS += -m64 -m80387 -msse -msse2 -mmmx \
          -mno-sse3 -mno-sse4 -mno-avx -mno-avx2 \
          -mno-avx512f -mcmodel=kernel \
          -mno-red-zone -fno-tree-vectorize
ARCH_LINK := arch/$(ARCH)/X86_64_link.lds
endif

.PHONY: archclean

archclean:
	rm -f arch/$(ARCH)/X86_64_link.lds

%.lds: %.ld
	@echo -e "CPP $@"
	$(Q)$(CPP) -P -x assembler-with-cpp -include include/generated/autoconf.h $^ -o $@
