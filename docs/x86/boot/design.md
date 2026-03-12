# NyxOS x86 boot path

## Image layout

The NyxOS bootable image contains two parts. An ELF executable (ET_EXEC or ET_REL) file containing the boot code, and an embedded ELF image of the kernel.

The ELF image of the kernel is surrounded by symbols `__kernel_blob_start` and `__kernel_blob_end`. Additionally `__kernel_blob_size` is also provided.

## Boot flow

The boot code is responsible for loading the kernel image into memory, mapping it's to it's runtime address, and preparing a `boot_params` structure for the kernel.

The kernel image must be loaded into physical memory respecting the offsets of the data inside the image (i.e. if the vaddr of the first segment is 0x1000, and the 2nd segment is 0x5000, then the 0x4000 byte spacing must be kept intact)

The boot code is then going to map the kernel to it's expected runtime address.

Next the boot code will prepare the `boot_params` structure. It's physical location must be page aligned, and it must be padded to a multiple of page size.

The boot code will then jump to the address indicated in `e_entry`. RDI will contain the physical address of the `boot_param` structure
