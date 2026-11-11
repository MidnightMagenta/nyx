local archinfo = {
	flags = {
		cflags = {
			"-m64",
			"-mno-sse",
			"-mno-mmx",
			"-msoft-float",
		},
	},
	link = mb.file("user.ld"),
}

return archinfo
