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
		"-Wenum-conversion",
	},
	["CONFIG_WARN_CONVERSION_ARITH$cflags"] = {
		"-Warith-conversion",
	},
	["CONFIG_WARN_BAD_FUNCTION_CAST$cflags"] = {
		"-Wbad-function-cast",
	},
	["CONFIG_WARN_TRADITIONAL_CONVERSION$cflags"] = {
		"-Wtraditional-conversion",
	},
	["CONFIG_WARN_UNSUFFIXED_FLOAT$cflags"] = {
		"-Wunsuffixed-float-constants",
	},

	-- ============================================================
	-- Casts, pointers, qualifiers
	-- ============================================================
	["CONFIG_WARN_CASTS$cflags"] = {
		"-Wcast-qual",
		"-Wpointer-arith",
	},
	["CONFIG_WARN_CAST_ALIGN_STRICT$cflags"] = {
		"-Wcast-align=strict",
	},
	["CONFIG_WARN_USELESS_CAST$cflags"] = {
		"-Wuseless-cast",
	},
	["CONFIG_WARN_WRITE_STRINGS$cflags"] = {
		"-Wwrite-strings",
	},
	["CONFIG_WARN_STRICT_ALIASING_MAX$cflags"] = {
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
	["CONFIG_WARN_DECL_AFTER_STATEMENT$cflags"] = {
		"-Wdeclaration-after-statement",
	},

	-- ============================================================
	-- Preprocessor
	-- ============================================================
	["CONFIG_WARN_PREPROCESSOR$cflags"] = {
		"-Wundef",
	},
	["CONFIG_WARN_UNUSED_MACROS$cflags"] = {
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
	["CONFIG_WARN_SWITCH_ENUM$cflags"] = {
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
	["CONFIG_WARN_FORMAT_SIGNEDNESS$cflags"] = {
		"-Wformat-signedness",
	},
	["CONFIG_WARN_FORMAT_OVERFLOW$cflags"] = {
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
	["CONFIG_WARN_STRICT_OVERFLOW$cflags"] = {
		"-Wstrict-overflow=5",
	},

	-- ============================================================
	-- Security-related constructs
	-- ============================================================
	["CONFIG_WARN_SECURITY$cflags"] = {
		"-Walloca",
		"-Wvla",
	},
	["CONFIG_WARN_STACK_PROTECTOR$cflags"] = {
		"-Wstack-protector",
	},
	["CONFIG_WARN_TRAMPOLINES$cflags"] = {
		"-Wtrampolines",
	},

	-- ============================================================
	-- Struct layout
	-- ============================================================
	["CONFIG_WARN_PACKED$cflags"] = {
		"-Wpacked",
	},
	["CONFIG_WARN_PADDED$cflags"] = {
		"-Wpadded",
	},

	-- ============================================================
	-- Optimizer-related
	-- ============================================================
	["CONFIG_WARN_INLINE$cflags"] = {
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
	["CONFIG_WARN_CXX_COMPAT$cflags"] = {
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
