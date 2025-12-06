CC ?= gcc
CXX ?= g++

CFLAGS := -O0 -g -MP -MMD -D_DEBUG
CXXFLAGS := $(CFLAGS) -std=c++17
CPPFLAGS :=
LDFLAGS := 

TARGET := build

obj := main.o evaluator.o toml_impl.o

ARGS ?=

.PHONY: all clean ccdb

all: $(TARGET)

$(TARGET): $(obj)
	@echo -e "LD $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	@find ./ -name "*.o" -type f -delete
	@rm -f compile_commands.json
	@rm -rf .cache
	@rm -f $(TARGET)
	@rm -f $(obj:.o=.d)

%.o: %.c
	@echo -e "CC $@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.o: %.cc
	@echo -e "CXX $@"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(obj:.o=.d)

ccdb:
	@compiledb make -Bn all
