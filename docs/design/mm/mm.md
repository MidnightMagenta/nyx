init*memory()
|* init*physmem()
| |* init*memmap()
| | |* sets up struct page\* mem*map
| |* init*zones()
| | |* sets up pg*data_t contigmem_page_data (initially. Later for NUMA more)  
| |* sets up the buddy allocator
|_ init_slab()
| |_ sets up the kernel's slab allocator
|_ init_virtmem()
| |_ sets up the virtual memory stuff
|\_TBD
|
