.PHONY: all clean 

all: pgdmp_x86

pgdmp_x86: pgdmp_x86.o 
	@echo -e "LD $@"
	@$(HOSTCC) -o $@ $^

clean:
	@find ./ -name "*.o" -type f -delete
	@rm -f compile_commands.json
	@rm -rf .cache
	@rm -f $(TARGET)
	@rm -f $(obj-lib:.o=.d)
	@rm -f $(obj:.o=.d)

%.o: %.c
	@echo -e "CC $@"
	@$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

-include $(obj:.o=.d)

