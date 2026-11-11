local mbl = require("mbuildlib")

local ldscript = mbl.bpath("X86_64_link.lds")

-- LD script generation
mb.build({
	rule = "gen_ld_script",
	out = mbl.bpath("X86_64_link.lds"),
	inputs = mb.file("X86_64_link.ld"),
	implicit = mbl.bpath("include/autoconf.h"),
})

-- ASM offsets generation
mb.rule("cc_asm_offsets", {
	command = "$cc $cflags -MMD -MP -MF $out.d -S $in -o $out",
	description = "CC $out",
	depfile = "$out.d",
	deps = "gcc",
})

mb.rule("gen_asm_offsets_h", {
	command = [[grep '^->' $in | sed 's/^->\([^ ]*\) \$$\?\(.*\)/#define \1 \2/' > $out]],
	description = "SED $out",
})

local asm_offsets_s = mb.build({
	rule = "cc_asm_offsets",
	out = mbl.bpath("arch/asmoffsets/asm-offsets.s"),
	inputs = mb.file("kernel/asm-offsets.c"),
	implicit = mbl.bpath("include/autoconf.h"),
	order_only = G_kernel_prerequisites,
})

local asm_offsets_h = mb.build({
	rule = "gen_asm_offsets_h",
	out = mbl.bpath("include/asi/generated/asm-offsets.h"),
	inputs = asm_offsets_s,
})

local archinfo = {
	flags = {
		cflags = {
			"-mcmodel=kernel",
			"-m64",
			"-mgeneral-regs-only",
			"-mno-red-zone",
			mbl.incl(mb.file("include")),
		},
		asflags = {
			"-m64",
		},
		ascppflags = {
			mbl.incl(mb.file("include")),
		},
	},
	arch_order_only = { asm_offsets_h },
	link = ldscript,
}

return archinfo
