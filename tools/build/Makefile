CC ?= gcc
CXX ?= g++
AR ?= ar

DEBUG ?= true
OPT_LVL ?= 0

LUA_DIR := lua-5.5.0
LUA_LIB := $(LUA_DIR)/src/liblua.a

CC_CXX_COMMON := -O$(OPT_LVL) -MP -MMD -I./$(LUA_DIR)/src

ifeq ($(DEBUG), true)
CCXXCOMMON += -g -D_DEBUG
endif

CFLAGS := $(CCXXCOMMON)
CXXFLAGS := $(CCXXCOMMON)
CPPFLAGS :=
ARFLAGS := rcs
LDFLAGS := -L./ -L./$(LUA_DIR)/src -lbuild

TARGET := build
LIB_TARGET := libbuild.a

obj := main.o
obj-lib := 

.PHONY: all clean ccdb

all: $(TARGET)

$(TARGET): $(obj) $(LIB_TARGET) $(LUA_LIB)
	@echo -e "LD $@"
	@$(CXX) $(LDFLAGS) -o $@ $^

$(LIB_TARGET): $(obj-lib)
	@echo -e "AR $@"
	@$(AR) $(ARFLAGS) $@ $^

$(LUA_LIB):
	@$(MAKE) -C $(LUA_DIR) all

clean:
	@find ./ -name "*.o" -type f -delete
	@rm -f compile_commands.json
	@rm -rf .cache
	@rm -f $(LIB_TARGET)
	@rm -f $(TARGET)
	@rm -f $(obj-lib:.o=.d)
	@rm -f $(obj:.o=.d)
	@$(MAKE) -C $(LUA_DIR) clean

%.o: %.c
	@echo -e "CC $@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.o: %.cc
	@echo -e "CXX $@"
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

-include $(obj:.o=.d)

ccdb:
	@compiledb make -Bn all

