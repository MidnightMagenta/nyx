local mbl = require("mbuildlib")

local ldscript = mbl.bpath("prekernel/boot_link.lds")

mb.build({
	rule = "gen_ld_script",
	out = ldscript,
	inputs = mb.file("boot_link.ld"),
	implicit = mbl.bpath("include/autoconf.h"),
})

local pkinfo = {
	flags = {
		cflags = {
			"-mgeneral-regs-only",
		},
		asflags = {},
	},
	-- objcopy output target for the embedded kernel blob
	bfdname = "elf64-x86-64",
	link = ldscript,
}

return pkinfo
