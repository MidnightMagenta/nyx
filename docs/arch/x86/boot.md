## Overview

This document describes the general boot flow for the x86_64 architecture.

The kernel's boot path consists of a small boot shim, responsible for:

- providing a minimal execution enviroment for the kernel
- loading the kernel into it's runtime address
- normalizing the boot information from whichever boot protocol was used into the kernel's own `boot_params` structure

The use of this design avoids the reliance of the kernel on any specific boot protocol. Since boot shims are small, one can relatively easily be implemented for any boot protocol. Additionally this design simplifies future implementation of certain kernel features, such as compressed kernels or KASLR.

## Machine state at handoff

At handoff the CPU should be in 64 bit long mode, with paging enabled, and PML5 paging (CR4.LA57) disabled. All conventional memory must be identity mapped, as well as direct mapped to the address `0xFFFF888000000000`. Additionally the kernel image must be mapped to it's runtime address. The kernel must be located in physical memory with respect to the offsets between segments. Interrupts must be disabled.

The physical address of the `boot_params` structure must be placed in `rdi` prior to jumping to the kernel's entry. The boot shim should ensure that the memory at that address is readable.

TODO: expand this documentation
TODO: add info about currently implemented shims
