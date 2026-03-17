## Build instructions

> [!INFO]
> Building this project requires the appropriate for your target cross compiler.

Prior to building the project, it is required to generate the appropriate configuration files. For this purpose run `make ARCH=[target architecture] config` or `make ARCH=[target architecture] menuconfig` for a terminal GUI configuration. Select the options that you need (if not sure, leave as default), and save the resulting configuration file as `.config` (default).

Once The kernel is configured to your need, simply run `make ARCH=[target architecture] CROSS_COMPILE=[your cross compiler prefix or blank] image`.
If you want to build only the virtual memory image of the kernel, run `make ARCH=[target architecture] CROSS_COMPILE=[your cross compiler prefix or blank] vmnyx`.

For convenience, a small script is provided for packaging and running the kernel in QEMU. You may use `./dbg mkimage` command to package the kernel, `./dbg run` to run the kernel in QEMU (requires a packaged image to be made first), or `./dbg run-debug` (or `./dbg run-debug-img` for debugging the boot shim) to launch GDB alongside QEMU.
