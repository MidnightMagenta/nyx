local _, filepath = ...
mb.track(filepath)

local extra_flags = {
	["CONFIG_DEBUG$cflags"] = {
		"-g",
	},
	["CONFIG_DEBUG$cppflags"] = {
		"-DDEBUG",
	},

	-- warnings
	["CONFIG_WARNINGS_AS_ERRORS$cflags"] = {
		"-Werror",
	},
	["CONFIG_WARN_EXTRA$cflags"] = {
		"-Wextra",
	},
	["CONFIG_WARN_PEDANTIC$cflags"] = {
		"-Wpedantic",
	},

	-- ============================================================
	-- Conversions
	-- ============================================================
	["CONFIG_WARN_CONVERSION$cflags"] = {
		"-Wconversion",
		"-Wsign-conversion",
		"-Wdouble-promotion",
		"-Wenum-conversion", -- GCC 10+
	},
	["CONFIG_WARN_CONVERSION_ARITH$cflags"] = {
		"-Warith-conversion", -- GCC 10+, only does something with -Wconversion
	},
	["CONFIG_WARN_BAD_FUNCTION_CAST$cflags"] = {
		"-Wbad-function-cast",
	},
	["CONFIG_WARN_TRADITIONAL_CONVERSION$cflags"] = { -- very noisy
		"-Wtraditional-conversion",
	},
	["CONFIG_WARN_UNSUFFIXED_FLOAT$cflags"] = { -- noisy
		"-Wunsuffixed-float-constants",
	},

	-- ============================================================
	-- Casts, pointers, qualifiers
	-- ============================================================
	["CONFIG_WARN_CASTS$cflags"] = {
		"-Wcast-qual",
		"-Wpointer-arith",
	},
	["CONFIG_WARN_CAST_ALIGN_STRICT$cflags"] = { -- noisy on strict-alignment-irrelevant targets (e.g. x86)
		"-Wcast-align=strict",
	},
	["CONFIG_WARN_USELESS_CAST$cflags"] = { -- C support needs GCC 14+
		"-Wuseless-cast",
	},
	["CONFIG_WARN_WRITE_STRINGS$cflags"] = { -- changes the type of string literals to const char[]
		"-Wwrite-strings",
	},
	["CONFIG_WARN_STRICT_ALIASING_MAX$cflags"] = { -- many false positives
		"-Wstrict-aliasing=1",
	},

	-- ============================================================
	-- Declarations, prototypes, scope
	-- ============================================================
	["CONFIG_WARN_DECLARATIONS$cflags"] = {
		"-Wshadow",
		"-Wredundant-decls",
		"-Wmissing-declarations",
		"-Wmissing-prototypes",
		"-Wstrict-prototypes",
		"-Wold-style-definition",
		"-Wnested-externs",
		"-Winit-self",
		"-Wjump-misses-init",
	},
	["CONFIG_WARN_DECL_AFTER_STATEMENT$cflags"] = { -- style: enforces C89-style declarations
		"-Wdeclaration-after-statement",
	},

	-- ============================================================
	-- Preprocessor
	-- ============================================================
	["CONFIG_WARN_PREPROCESSOR$cflags"] = {
		"-Wundef",
	},
	["CONFIG_WARN_UNUSED_MACROS$cflags"] = { -- noisy in headers/shared macro files
		"-Wunused-macros",
	},

	-- ============================================================
	-- Control flow / logic
	-- ============================================================
	["CONFIG_WARN_CONTROL_FLOW$cflags"] = {
		"-Wduplicated-cond",
		"-Wduplicated-branches",
		"-Wlogical-op",
		"-Wimplicit-fallthrough=5",
	},
	["CONFIG_WARN_SWITCH_DEFAULT$cflags"] = {
		"-Wswitch-default",
	},
	["CONFIG_WARN_SWITCH_ENUM$cflags"] = { -- noisy: every enum value must be listed even with default
		"-Wswitch-enum",
	},
	["CONFIG_WARN_FLOAT_EQUAL$cflags"] = {
		"-Wfloat-equal",
	},

	-- ============================================================
	-- Format strings
	-- ============================================================
	["CONFIG_WARN_FORMAT$cflags"] = {
		"-Wformat=2",
	},
	["CONFIG_WARN_FORMAT_SIGNEDNESS$cflags"] = { -- moderately noisy
		"-Wformat-signedness",
	},
	["CONFIG_WARN_FORMAT_OVERFLOW$cflags"] = { -- needs -O1+
		"-Wformat-overflow=2",
		"-Wformat-truncation=2",
	},

	-- ============================================================
	-- Bounds / overflow analysis (all need -O1+, ideally -O2)
	-- ============================================================
	["CONFIG_WARN_BOUNDS$cflags"] = {
		"-Wnull-dereference",
		"-Warray-bounds=2",
		"-Wstringop-overflow=4",
		"-Wshift-overflow=2",
	},
	["CONFIG_WARN_STRICT_OVERFLOW$cflags"] = { -- very noisy at level 5
		"-Wstrict-overflow=5",
	},

	-- ============================================================
	-- Security-related constructs
	-- ============================================================
	["CONFIG_WARN_SECURITY$cflags"] = {
		"-Walloca",
		"-Wvla",
	},
	["CONFIG_WARN_STACK_PROTECTOR$cflags"] = { -- only meaningful with -fstack-protector*
		"-Wstack-protector",
	},
	["CONFIG_WARN_TRAMPOLINES$cflags"] = { -- only relevant if you use nested functions
		"-Wtrampolines",
	},

	-- ============================================================
	-- Struct layout
	-- ============================================================
	["CONFIG_WARN_PACKED$cflags"] = {
		"-Wpacked",
	},
	["CONFIG_WARN_PADDED$cflags"] = { -- fires on nearly every struct
		"-Wpadded",
	},

	-- ============================================================
	-- Optimizer-related
	-- ============================================================
	["CONFIG_WARN_INLINE$cflags"] = { -- rarely useful
		"-Winline",
	},
	["CONFIG_WARN_DISABLED_OPT$cflags"] = {
		"-Wdisabled-optimization",
	},
	["CONFIG_WARN_UNSAFE_LOOP_OPT$cflags"] = {
		"-Wunsafe-loop-optimizations",
	},

	-- ============================================================
	-- C++ compatibility
	-- ============================================================
	["CONFIG_WARN_CXX_COMPAT$cflags"] = { -- flags valid C that wouldn't compile as C++
		"-Wc++-compat",
	},

	-- ============================================================
	-- Build environment hygiene
	-- ============================================================
	["CONFIG_WARN_BUILD_HYGIENE$cflags"] = {
		"-Wmissing-include-dirs",
		"-Wdate-time",
		"-Winvalid-pch",
	},
}

return extra_flags
